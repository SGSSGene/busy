#include "utils.h"

#include "toolchains.h"
#include "overloaded.h"

#include <iostream>
#include <process/Process.h>
#include <queue>


namespace busy {

void printProjects(std::map<analyse::Project const*, std::tuple<std::set<analyse::Project const*>, std::set<analyse::Project const*>>> const& _projects) {
	for (auto const& [i_project, dep] : _projects) {
		auto const& project = *i_project;
		auto const& dependencies = std::get<0>(dep);
		std::cout << "\n";

		std::cout << "  - project-name: " << project.getName() << "\n";
		std::cout << "    path: " << project.getPath() << "\n";
		if (not project.getSystemLibraries().empty()) {
			std::cout << "    systemLibraries:\n";
			for (auto const& l : project.getSystemLibraries()) {
				std::cout << "    - " << l << "\n";
			}
		}
		if (not dependencies.empty()) {
			std::cout << "    dependencies:\n";
			for (auto const& d : dependencies) {
				std::cout << "    - name: " << d->getName() << "\n";
				std::cout << "      path: " << d->getPath() << "\n";
			}
		}
		if (not project.getLegacyIncludePaths().empty()) {
			std::cout << "    includePaths:\n";
			for (auto const& p : project.getLegacyIncludePaths()) {
				std::cout << "      - " << p << "\n";
			}
		}
	}
}

auto loadConfig(std::filesystem::path workPath, std::optional<std::filesystem::path> rootPath) -> Config {
	auto config = [&]() {
		if (exists(global_busyConfigFile)) {
			return fon::yaml::deserialize<Config>(YAML::LoadFile(global_busyConfigFile));
		}
		return Config{};
	}();
	if (rootPath) {
		config.rootDir = relative(workPath / *rootPath);
	}

	if (config.rootDir.empty()) {
		throw std::runtime_error("please give path of busy.yaml");
	}

	if (config.rootDir == ".") {
		throw std::runtime_error("can't build in source, please create a `build` directory");
	}
	return config;
}

auto updateToolchainOptions(Config& config, bool reset, std::optional<std::vector<std::string>> _options) -> std::map<std::string, std::vector<std::string>> {
	auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);

	// initialize queue
	auto queue = std::queue<std::string>{};
	auto processed = std::set<std::string>{};
	if (reset) {
		queue.push("default");
		config.toolchain.options.clear();
	}
	if (_options) {
		for (auto o : *_options) {
			queue.push(o);
		}
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
			std::cout << "unknown toolchain option " << o << " (removed)\n";
		} else {
			if (act) {
				config.toolchain.options.insert(opt);
				for (auto o2 : iter->second) {
					queue.push(o2);
				}
			} else {
				config.toolchain.options.erase(opt);
			}
		}
	}
	return toolchainOptions;
}

auto computeEstimationTimes(Config const& config, analyse::ProjectMap const& projects_with_deps, bool clean, int jobs) -> std::tuple<ConsolePrinter::EstimatedTimes, std::chrono::milliseconds> {
	auto estimatedTimes = ConsolePrinter::EstimatedTimes{};
	auto pipe           = analyse::CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};
	auto threadTimings = std::vector<std::chrono::milliseconds>(jobs, std::chrono::milliseconds{0});
	auto readyTimings  = std::vector<std::chrono::milliseconds>{};
	for (int i{0}; i < pipe.size(); ++i) {
		readyTimings.push_back(std::chrono::milliseconds{0});
	}
	while (not pipe.empty()) {
		auto work = pipe.pop();
		auto duration = std::chrono::milliseconds{};
		int otherProcesses = pipe.size();
		pipe.dispatch(work, overloaded {
			[&](busy::analyse::File const& file, auto const& params, auto const& deps) {
				auto& fileInfo = getFileInfos().get(file.getPath());
				if (clean or (fileInfo.hasChanged() and fileInfo.compilable)) {
					estimatedTimes.try_emplace(&file, fileInfo.compileTime);
					duration = fileInfo.compileTime;
				}
				return analyse::CompilePipe::Color::Compilable;
			}, [&](busy::analyse::Project const& project, auto const& params, auto const& deps) {
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
				if (clean or (anyChanges())) {
					estimatedTimes.try_emplace(&project, fileInfo.compileTime);
					duration = fileInfo.compileTime;
				}
				return analyse::CompilePipe::Color::Compilable;
			}
		});
		// find smallest thread with timings
		threadTimings[0] = std::max(readyTimings.front(), threadTimings.front()) + duration;
		readyTimings.erase(begin(readyTimings));
		auto newProcesses = pipe.size() - otherProcesses;
		for (auto i{0}; i < newProcesses; ++i) {
			readyTimings.push_back(threadTimings[0]);
		}
		std::sort(begin(threadTimings), end(threadTimings));
		std::sort(begin(readyTimings), end(readyTimings));
	}

	return {estimatedTimes, threadTimings.back()};
}

auto execute(std::vector<std::string> const& params, bool verbose) -> std::string {
	auto p = process::Process{params};
	auto call = std::stringstream{};
	for (auto const& p : params) {
		call << p << " ";
	}

	if (verbose or p.getStatus() != 0) {
		std::cout << "call: " << call.str() << "\n";
	}
	if (p.getStatus() != 0) {
		std::cout << p.cout() << "\n";
		std::cerr << p.cerr() << "\n";
		throw CompileError{};
	}
	return p.cout();
};


}
