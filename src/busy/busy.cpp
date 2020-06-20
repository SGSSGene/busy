#include "Queue.h"
#include "utils/utils.h"
#include "cache.h"
#include "config.h"
#include "toolchains.h"
#include "overloaded.h"
#include "analyse.h"
#include "ConsolePrinter.h"
#include "utils.h"
#include "MultiCompilePipe.h"

#include <algorithm>
#include <cstdlib>
#include <sargparse/ArgumentParsing.h>
#include <sargparse/Parameter.h>
#include <sargparse/File.h>
#include <yaml-cpp/yaml.h>

namespace busy::analyse {

auto cfgVerbose   = sargp::Flag{"verbose", "verbose output"};
auto cfgRootPath  = sargp::Parameter<sargp::Directory>{"..", "root",  "path to directory containing busy.yaml"};
auto cfgBuildPath = sargp::Parameter<sargp::Directory>{".",  "build", "path to build directory"};

void listToolchains(std::vector<std::filesystem::path> const& packages) {
	for (auto [name, path]  : searchForToolchains(packages)) {
		std::cout << "  - " << name << " (" << path << ")\n";
	}
}

auto cmdLsToolchains = sargp::Command{"ls-toolchains", "list all available toolchains", []() {
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});

	auto packages = std::vector<std::filesystem::path>{};
	if (!config.rootDir.empty() and config.rootDir != "." and std::filesystem::exists(config.rootDir)) {
		auto [pro, pack] = busy::analyse::readPackage(config.rootDir, ".");
		for (auto p : pack) {
			packages.emplace_back(p);
		}
	}

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	listToolchains(packages);
}};

auto cfgOptions = sargp::Parameter<std::vector<std::string>>{{}, "option", "options for toolchains", [](){}, [](std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>> {
	auto ret = std::pair<bool, std::set<std::string>>{false, {}};
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});
	auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);
	for (auto opt : toolchainOptions) {
		if (config.toolchain.options.count(opt.first) == 0) {
			ret.second.insert(opt.first);
		} else {
			ret.second.insert("no-" + opt.first);
		}
	}
	return ret;
}};
auto cfgToolchain = sargp::Parameter<std::string>{"", "toolchain", "set toolchain", [](){}, [](std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>> {
	auto ret = std::pair<bool, std::set<std::string>>{false, {}};
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});

	auto packages = std::vector<std::filesystem::path>{};
	if (!config.rootDir.empty() and config.rootDir != "." and std::filesystem::exists(config.rootDir)) {
		auto [pro, pack] = busy::analyse::readPackage(config.rootDir, ".");
		for (auto p : pack) {
			packages.emplace_back(p);
		}
	}

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);
	for (auto const&  [name, path] : searchForToolchains(packages)) {
		ret.second.insert(name);
	}
	return ret;
}};

auto cmdShowDeps = sargp::Command{"show-deps", "show dependencies of projects", []() {
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});

	auto [projects, packages] = busy::analyse::readPackage(config.rootDir, ".");

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	// check consistency of packages
	std::cout << "checking consistency..." << std::flush;
	checkConsistency(projects);
	std::cout << "done\n";

	auto projects_with_deps = createProjects(projects);
	printProjects(projects_with_deps);

}};

auto cmdVersionShow = []() {
	std::cout << "busy 2.0.0-git-alpha\n";
	std::cout << "Copyright (C) 2020 Simon Gene Gottlieb\n";
};
auto cmdVersion   = sargp::Command{"version", "show version", cmdVersionShow};
auto cfgVersion   = sargp::Flag{"version", "show version", cmdVersionShow};
auto cfgClean     = sargp::Flag{"clean", "clean build, using no cache"};
auto cfgYamlCache = sargp::Flag{"yaml-cache", "save cache in yaml format"};
auto cfgJobs      = sargp::Parameter<int>{0, "jobs", "thread count"};

void app() {
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});
	auto cacheGuard = loadFileCache(*cfgYamlCache);

	auto [projects, packages] = busy::analyse::readPackage(config.rootDir, ".");

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);


	if (cfgToolchain) {
		auto toolchains = searchForToolchains(packages);
		auto iter = toolchains.find(*cfgToolchain);
		if (iter == toolchains.end()) {
			throw std::runtime_error("could not find toolchain \"" + config.toolchain.name + "\"");
		}

		config.toolchain.name = iter->first;
		config.toolchain.call = iter->second;
		updateToolchainOptions(config, true, cfgOptions);
		std::cout << "setting toolchain to " << config.toolchain.name << " (" << config.toolchain.call << ")\n";
	}
	auto toolchainOptions = updateToolchainOptions(config, false, cfgOptions);

	std::cout << "using toolchain " << config.toolchain.name << " (" << config.toolchain.call << ")\n";
	std::cout << "  with options: ";
	for (auto const& o : config.toolchain.options) {
		std::cout << o << " ";
	}
	std::cout << "\n";


	// check consistency of packages
	std::cout << "checking consistency..." << std::flush;
	checkConsistency(projects);
	std::cout << "done\n";

	auto projects_with_deps = createProjects(projects);

	// Save config
	{
		YAML::Emitter out;
		out << fon::yaml::serialize(config);
		std::ofstream(global_busyConfigFile) << out.c_str();
	}


	std::cout << "checking files...";
	auto jobs = [&]() -> int {
		if (*cfgJobs == 0) {
			return std::thread::hardware_concurrency();
		}
		return *cfgJobs;
	}();

	bool clean = cfgClean;
	{
		auto cout = execute({config.toolchain.call, "begin", config.rootDir}, false);
		auto node = YAML::Load(cout);
		clean = YAML::Node{node["clean"]}.as<bool>(clean);
		jobs = std::min(jobs, YAML::Node{node["max_jobs"]}.as<int>(jobs));
	}

	auto [estimatedTimes, estimatedTotalTime] = computeEstimationTimes(config, projects_with_deps, clean, jobs);
	std::cout << "done\n";
	if (estimatedTimes.empty()) {
		return;
	}


	[&]() {
		std::cout << "start compiling...\n";
		auto consolePrinter = ConsolePrinter{estimatedTimes, estimatedTotalTime};
		auto pipe           = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};

		auto multiPipe = MultiCompilePipe{pipe, jobs};

		std::mutex mutex;
		multiPipe.work(overloaded {
			[&](busy::analyse::File const& file, auto const& params, auto const& deps) {
				auto startTime  = std::chrono::file_clock::now();
				auto path       = file.getPath();

				auto g = std::unique_lock{mutex};
				auto& fileInfo = getFileInfos().get(path);
				g.unlock();
				if (estimatedTimes.count(&file) == 0) {
					return fileInfo.compilable?analyse::CompilePipe::Color::Compilable:analyse::CompilePipe::Color::Ignored;
				}

				consolePrinter.startJob(&file, "compiling " + path.string());

				fileInfo.needRecompiling = true;
				// store file date before compiling
				g.lock();
				auto hash    = getFileCache().getHash(path);
				g.unlock();

				// compiling
				auto cout = execute(params, cfgVerbose);
				auto node = YAML::Load(cout);

				bool cached         = YAML::Node{node["cached"]}.as<bool>(false);
				fileInfo.compilable = YAML::Node{node["compilable"]}.as<bool>(false);

				auto dependencies = FileInfo::Dependencies{};
				//!TODO should work with `auto`, but doesn't for some reason
				for (YAML::Node n : node["dependencies"]) {
					auto path = FileInfo::Path{n.as<std::string>()};
					auto hash    = computeHash(path);
					auto modTime = getFileModificationTime(path);

					//!TODO handle when files changes inbetween
					if (modTime > startTime) {
						auto f = [](auto t) {
							return std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
						};
							std::cout << "file changed??? " << f(modTime) << " " << f(startTime) << " " << path << " " << std::filesystem::current_path() << " " << hash << "\n";
					//		throw std::runtime_error("file changed");
						//status = 2; // file changed in between
					}
					dependencies.emplace_back(path, hash);
				}
				fileInfo.dependencies    = dependencies;
				fileInfo.hash            = hash;
				fileInfo.needRecompiling = false;

				auto compileTime = consolePrinter.finishedJob(&file);
				if (not cached or fileInfo.compileTime < compileTime) {
					fileInfo.compileTime = compileTime;
				}
				fileInfo.modTime = startTime;
				return fileInfo.compilable?analyse::CompilePipe::Color::Compilable:analyse::CompilePipe::Color::Ignored;
			}, [&](busy::analyse::Project const& project, auto const& params, auto const& deps) {
				auto& fileInfo = getFileInfos().get(project.getPath());

				if (estimatedTimes.count(&project) == 0) {
					return fileInfo.compilable?analyse::CompilePipe::Color::Compilable:analyse::CompilePipe::Color::Ignored;
				}

				consolePrinter.startJob(&project, "linking " + project.getName());
				auto cout = execute(params, cfgVerbose);
				auto node = YAML::Load(cout);
				bool cached         = YAML::Node{node["cached"]}.as<bool>(false);
				fileInfo.compilable = YAML::Node{node["compilable"]}.as<bool>(false);

				auto compileTime = consolePrinter.finishedJob(&project);
				if (not cached or fileInfo.compileTime < compileTime) {
					fileInfo.compileTime = compileTime;
				}

				return fileInfo.compilable?analyse::CompilePipe::Color::Compilable:analyse::CompilePipe::Color::Ignored;
			}
		});
		multiPipe.join();

		execute({config.toolchain.call, "end"}, false);
	}();
	std::cout << "done\n";
}

auto cmdCompile = sargp::Command{"compile", "compile everything (default)", []() {
	app();
}};
auto cmdCompileDefault = sargp::Task{[]{
	if (cmdLsToolchains or cmdShowDeps or cmdVersion or cfgVersion) return;
	app();
}};

}

namespace {
	auto printHelp  = sargp::Parameter<std::optional<std::string>>{{}, "help", "print this help - add a string for  grep-like search"};
}

int main(int argc, char const** argv) {
	try {
		if (std::string_view{argv[argc-1]} == "--bash_completion") {
			auto hint = sargp::compgen(argc-2, argv+1);
			std::cout << hint;
			return 0;
		}

		sargp::parseArguments(argc-1, argv+1);
		if (printHelp) {
			std::cout << sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"});
			return 0;
		}
		sargp::callCommands();
		return EXIT_SUCCESS;
	} catch (busy::CompileError const& e) {
	} catch (std::exception const& e) {
		std::cerr << "exception: " << busy::utils::exceptionToString(e, 0) << "\n";
		std::cerr << sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"});
	}
	return EXIT_FAILURE;
}
