#include "utils.h"

#include "toolchains.h"
#include "overloaded.h"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>
#include <process/Process.h>
#include <queue>
#include <unistd.h>
#include <sys/file.h>


namespace busy {

FileLock::FileLock() {
	fd = ::open(global_lockFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
		throw std::runtime_error("can't access .lock");
	}
	if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
		throw std::runtime_error("other process is running");
	}
}
FileLock::~FileLock() {
	close(fd);
	remove(global_lockFile);
}


void printProjects(std::map<Project const*, std::tuple<std::set<Project const*>, std::set<Project const*>>> const& _projects) {
	for (auto const& [i_project, dep] : _projects) {
		auto const& project = *i_project;
		auto const& dependencies = std::get<0>(dep);
		fmt::print("\n");

		fmt::print("  - project-name: {}\n", project.getName());
		fmt::print("    path: {}\n", project.getPath());
		if (not project.getSystemLibraries().empty()) {
			fmt::print("    systemLibraries:\n");
			for (auto const& l : project.getSystemLibraries()) {
				fmt::print("   - {}\n", l);
			}
		}
		if (not dependencies.empty()) {
			fmt::print("    dependencies:\n");
			for (auto const& d : dependencies) {
				fmt::print("    - name: {}\n", d->getName());
				fmt::print("      path: {}\n", d->getPath());
			}
		}
		if (not project.getLegacyIncludePaths().empty()) {
			fmt::print("    includePath:\n");
			for (auto const& p : project.getLegacyIncludePaths()) {
				fmt::print("    - {}\n", p);
			}
		}
	}
}

auto loadConfig(std::filesystem::path const& workPath, std::filesystem::path const& buildPath, std::tuple<bool, std::filesystem::path> const& rootPath) -> Config {

	if (relative(buildPath) != ".") {
		if (not exists(buildPath)) {
			std::filesystem::create_directories(buildPath);
		}
		std::filesystem::current_path(buildPath);
		fmt::print("changing working directory to {}\n", buildPath);
	}

	auto config = [&]() {
		if (exists(global_busyConfigFile)) {
			return fon::yaml::deserialize<Config>(YAML::LoadFile(global_busyConfigFile));
		}
		return Config{};
	}();
	if (std::get<0>(rootPath) or config.rootDir.empty()) {
		config.rootDir = relative(workPath / std::get<1>(rootPath));
	}

	if (config.rootDir.empty()) {
		throw std::runtime_error("please give path of busy.yaml");
	}

	if (config.rootDir == ".") {
		throw std::runtime_error("can't build in source, please create a `build` directory");
	}
	return config;
}

auto updateToolchainOptions(Config& config, bool reset, std::vector<std::string> const& _options) -> std::map<std::string, std::vector<std::string>> {
	auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);

	// initialize queue
	auto queue = std::queue<std::string>{};
	auto processed = std::set<std::string>{};
	if (reset) {
		queue.push("default");
		config.toolchain.options.clear();
	}
	for (auto const& o : _options) {
		queue.push(o);
	}
	while(not queue.empty()) {
		auto o = queue.front();
		queue.pop();

		auto [opt, act] = [&]() -> std::tuple<std::string, bool> {
			if (o.length() > 3 and o.substr(0, 3) == "no-") {
				return {o.substr(3), false};
			}
			return {o, true};
		}();

		if (processed.count(o) > 0) continue;
		processed.insert(o);

		auto iter = toolchainOptions.find(opt);
		if (iter == toolchainOptions.end()) {
			fmt::print("unknown toolchain option {} (removed)\n", o);
		} else {
			if (act) {
				config.toolchain.options.insert(opt);
				for (auto const& o2 : iter->second) {
					queue.push(o2);
				}
			} else {
				config.toolchain.options.erase(opt);
			}
		}
	}
	return toolchainOptions;
}

auto computeEstimationTimes(Config const& config, ProjectMap const& projects_with_deps, bool clean, std::string const& _compilerHash, std::size_t jobs) -> std::tuple<ConsolePrinter::EstimatedTimes, std::chrono::milliseconds> {
	auto estimatedTimes = ConsolePrinter::EstimatedTimes{};
	auto pipe           = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};
	auto threadTimings = std::vector<std::chrono::milliseconds>(jobs, std::chrono::milliseconds{0});
	auto readyTimings  = std::vector<std::chrono::milliseconds>{};
	for (std::size_t i{0}; i < pipe.size(); ++i) {
		readyTimings.emplace_back(std::chrono::milliseconds{0});
	}
	while (not pipe.empty()) {
		auto work = pipe.pop();
		auto duration = std::chrono::milliseconds{};
		auto otherProcesses = pipe.size();
		pipe.dispatch(work, overloaded {
			[&](busy::File const& file, auto const& /*params*/, auto const& /*deps*/) {
				auto& fileInfo = getFileInfos().get(file.getPath());
				if (clean
				    or (fileInfo.hasChanged() and fileInfo.compilable)
				    or fileInfo.compilerHash != _compilerHash) {
					estimatedTimes.try_emplace(&file, fileInfo.compileTime);
					duration = fileInfo.compileTime;
				}
				return CompilePipe::Color::Compilable;
			}, [&](busy::Project const& project, auto const& /*params*/, auto const& deps) {
				auto& fileInfo = getFileInfos().get(project.getPath());
				auto anyChanges = [&]() {
					for (auto const& d : deps) {
						if (estimatedTimes.count(d) > 0) {
							return true;
						}
					}
					for (auto const& file : project.getFiles()) {
						if (estimatedTimes.count(&file) > 0) {
							return true;
						}
					}
					return false;
				};
				auto existsAllOutputFiles = [&]() {
					for (auto const& outputFile : fileInfo.outputFiles) {
						if (not exists(outputFile)) {
							return false;
						}
					}
					return true;
				}();
				if (clean 
				    or (not existsAllOutputFiles and fileInfo.compilable)
				    or anyChanges()
				    or fileInfo.compilerHash != _compilerHash) {
					estimatedTimes.try_emplace(&project, fileInfo.compileTime);
					duration = fileInfo.compileTime;
				}
				return CompilePipe::Color::Compilable;
			}
		});
		// find smallest thread with timings
		threadTimings[0] = std::max(readyTimings.front(), threadTimings.front()) + duration;
		readyTimings.erase(begin(readyTimings));
		auto newProcesses = pipe.size() - otherProcesses;
		for (std::size_t i{0}; i < newProcesses; ++i) {
			readyTimings.push_back(threadTimings[0]);
		}
		std::sort(begin(threadTimings), end(threadTimings));
		std::sort(begin(readyTimings), end(readyTimings));
	}

	return {estimatedTimes, threadTimings.back()};
}

auto execute(std::vector<std::string> params, bool verbose) -> std::string {
	if (verbose) {
		params.emplace_back("-verbose");
	}
	auto call = std::stringstream{};
	for (auto const& p : params) {
		call << p << " ";
	}
	if (verbose) {
		fmt::print("call: {}\n", call.str());
	}

	auto p = process::Process{params};

	if (not verbose and p.getStatus() != 0) {
		fmt::print("call: {}\n", call.str());
	}
	if (verbose or p.getStatus() != 0) {
		if (not p.cout().empty()) fmt::print(std::cout, "{}\n", p.cout());
		if (not p.cerr().empty()) fmt::print(std::cerr, "{}\n", p.cerr());
	}
	if (p.getStatus() != 0) {
		throw CompileError{};
	}
	return p.cout();
}

void visitFilesWithWarnings(Config const& config, ProjectMap const& projects_with_deps, std::function<void(File const&, FileInfo const&)> fileF, std::function<void(Project const&, FileInfo const&)> projectF) {
	auto pipe = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};
	while (not pipe.empty()) {
		auto work = pipe.pop();
		pipe.dispatch(work, overloaded {
			[&](busy::File const& file, auto const& /*params*/, auto const& /*deps*/) {
				auto& fileInfo = getFileInfos().get(file.getPath());
				if (fileInfo.hasWarnings and fileF) {
					fileF(file, fileInfo);
				}
				return CompilePipe::Color::Compilable;
			}, [&](busy::Project const& project, auto const& /*params*/, auto const& /*deps*/) {
				auto& fileInfo = getFileInfos().get(project.getPath());
				if (fileInfo.hasWarnings and projectF) {
					projectF(project, fileInfo);
				}
				return CompilePipe::Color::Compilable;
			}
		});
	}
}

auto isInteractive() -> bool {
	static bool interactive = isatty(fileno(stdout));
	return interactive;

}

}
