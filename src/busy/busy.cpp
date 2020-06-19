#include "Queue.h"
#include "utils/utils.h"
#include "cache.h"
#include "config.h"
#include "toolchains.h"
#include "overloaded.h"
#include "analyse.h"
#include "ConsolePrinter.h"


#include <algorithm>
#include <cstdlib>
#include <process/Process.h>
#include <sargparse/ArgumentParsing.h>
#include <sargparse/Parameter.h>
#include <sargparse/File.h>
#include <yaml-cpp/yaml.h>

namespace busy::analyse {

auto cfgVerbose   = sargp::Flag{"verbose", "verbose output"};
auto cfgRootPath  = sargp::Parameter<sargp::Directory>{"..", "root",  "path to directory containing busy.yaml"};
auto cfgBuildPath = sargp::Parameter<sargp::Directory>{".",  "build", "path to build directory"};

void printProjects(std::map<Project const*, std::tuple<std::set<Project const*>, std::set<Project const*>>> const& _projects) {
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

void listToolchains(std::vector<std::filesystem::path> const& packages) {
	for (auto [name, path]  : searchForToolchains(packages)) {
		std::cout << "  - " << name << " (" << path << ")\n";
	}
}

auto loadConfig(std::filesystem::path workPath) {
	auto config = [&]() {
		if (exists(global_busyConfigFile)) {
			return fon::yaml::deserialize<Config>(YAML::LoadFile(global_busyConfigFile));
		}
		return Config{};
	}();
	if (cfgRootPath) {
		config.rootDir = relative(workPath / *cfgRootPath);
	}

	if (config.rootDir.empty()) {
		throw std::runtime_error("please give path of busy.yaml");
	}

	if (config.rootDir == ".") {
		throw std::runtime_error("can't build in source, please create a `build` directory");
	}
	return config;
}


auto cmdLsToolchains = sargp::Command{"ls-toolchains", "list all available toolchains", []() {
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath);

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
	auto config = loadConfig(workPath);
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
	auto config = loadConfig(workPath);

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

auto updateToolchainOptions(Config& config, bool reset) -> std::map<std::string, std::vector<std::string>> {
	auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);

	// initialize queue
	auto queue = std::queue<std::string>{};
	auto processed = std::set<std::string>{};
	if (reset) {
		queue.push("default");
		config.toolchain.options.clear();
	}
	for (auto o : *cfgOptions) {
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

auto cmdShowDeps = sargp::Command{"show-deps", "show dependencies of projects", []() {
	auto workPath = std::filesystem::current_path();

	if (cfgBuildPath and relative(std::filesystem::path{*cfgBuildPath}) != ".") {
		std::filesystem::create_directories(*cfgBuildPath);
		std::filesystem::current_path(*cfgBuildPath);
		std::cout << "changing working directory to " << *cfgBuildPath << "\n";
	}
	auto config = loadConfig(workPath);

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
auto cmdVersion  = sargp::Command{"version", "show version", cmdVersionShow};
auto cfgVersion  = sargp::Flag{"version", "show version", cmdVersionShow};
auto cfgClean    = sargp::Flag{"clean", "clean build, using no cache"};
auto cfgYamlCache = sargp::Flag{"yaml-cache", "save cache in yaml format"};

void app() {
	auto workPath = std::filesystem::current_path();

	if (cfgBuildPath and relative(std::filesystem::path{*cfgBuildPath}) != ".") {
		std::filesystem::create_directories(*cfgBuildPath);
		std::filesystem::current_path(*cfgBuildPath);
		std::cout << "changing working directory to " << *cfgBuildPath << "\n";
	}
	auto config = loadConfig(workPath);
	loadFileCache();

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
		updateToolchainOptions(config, true);
		std::cout << "setting toolchain to " << config.toolchain.name << " (" << config.toolchain.call << ")\n";
	}
	auto toolchainOptions = updateToolchainOptions(config, false);

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
	//printProjects(projects_with_deps);

	// Save config
	{
		YAML::Emitter out;
		out << fon::yaml::serialize(config);
		std::ofstream(global_busyConfigFile) << out.c_str();
	}


	std::cout << "checking files...\n";
	[&](){
		auto estimatedTimes      = ConsolePrinter::EstimatedTimes{};

		// precompute timings
		{
			auto pipe = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};
			while (not pipe.empty()) {
				pipe.dispatch(overloaded {
					[&](busy::analyse::File const& file, auto const& params, auto const& deps) {
						auto& fileInfo = getFileInfos().get(file.getPath());
						if (*cfgClean or (fileInfo.hasChanged() and fileInfo.compilable)) {
							estimatedTimes.try_emplace(&file, fileInfo.compileTime);
						}
						return 0;
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
						if (*cfgClean or (anyChanges() and fileInfo.compilable)) {
							estimatedTimes.try_emplace(&project, fileInfo.compileTime);
						}
						return 0;
					}
				});
			}
		}
		if (estimatedTimes.empty()) {
			return;
		}

		std::cout << "start compiling...\n";
		auto consolePrinter = ConsolePrinter{estimatedTimes};

		auto pipe = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};

		auto runToolchain = [&](std::string_view str) {
			auto params = std::vector<std::string>{config.toolchain.call, std::string{str}};
			auto p = process::Process{params};

			if (p.getStatus() != 0) {
				throw std::runtime_error{"failed running toolchain " + std::string{str}};
			}
		};

		runToolchain("begin");

		auto execute = [&](auto const& params) -> std::tuple<int, std::string> {
			auto p = process::Process{params};
			if (*cfgVerbose) {
				std::cout << "call:";
				for (auto p : params) {
					std::cout << " " << p;
				}
				std::cout << "\n";
			}
			if (p.getStatus() != 0 and p.getStatus() != 1) {
				std::stringstream ss;
				for (auto const& p : params) {
					ss << p << " ";
				}
				std::cout << ss.str() << "\n";
				std::cout << p.cout() << "\n";
				std::cerr << p.cerr() << "\n";
				if (p.getStatus() != 0 and p.getStatus() != 1) {
					std::cout << "error exit\n";
					exit(1);
				}
			}
			if (p.getStatus() == 1) {
				return {1, ""};
			}
			return {0, p.cout()};
		};

		while (not pipe.empty()) {
			pipe.dispatch(overloaded {
				[&](busy::analyse::File const& file, auto const& params, auto const& deps) {
					auto startTime  = std::chrono::file_clock::now();
					auto path = file.getPath();

					auto& fileInfo = getFileInfos().get(path);

					// check if file needs recompile
					auto status = [&]() {
						if (estimatedTimes.count(&file) > 0) {
							consolePrinter.startJob(&file, "compiling " + path.string());

							fileInfo.needRecompiling = true;
							// store file date before compiling
							auto hash    = getFileCache().getHash(path);

							// compiling
							auto [status, cout] = execute(params);
							auto dependencies = FileInfo::Dependencies{};
							bool cached = false;
							if (status == 0) {
								auto node = YAML::Load(cout);
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
//										throw std::runtime_error("file changed");
										//status = 2; // file changed in between
									}
									dependencies.emplace_back(path, hash);
								}
								if (YAML::Node{node["cached"]}.as<bool>()) {
									cached = true;
								}
								fileInfo.dependencies = dependencies;
							} else if (status == 1) {
								fileInfo.compilable = false;
							}
							// update caches
							fileInfo.hash = hash;

							if (status == 0) {
								fileInfo.needRecompiling = false;
							}

							auto compileTime = consolePrinter.finishedJob(&file);
							if (not cached or fileInfo.compileTime < compileTime) {
								fileInfo.compileTime = compileTime;
							}
							return status;
						} else {
							//std::cout << "\n===\nno change - skip - " << file.getPath() << "\n===\n";
							if (not fileInfo.compilable) {
								return 1;
							}
							return 0;
						}
					}();
					auto stopTimer = std::chrono::steady_clock::now();
					fileInfo.modTime     = startTime;
					return status;
				}, [&](busy::analyse::Project const& project, auto const& params, auto const& deps) {
					auto path = project.getPath();
					auto& fileInfo = getFileInfos().get(path);

					if (estimatedTimes.count(&project) > 0) {
						consolePrinter.startJob(&project, "linking " + path.string());
						auto [status, cout] = execute(params);
						bool cached = false;

						if (status == 0) {
							auto node = YAML::Load(cout);
							if (YAML::Node{node["cached"]}.as<bool>()) {
								cached = true;
							}

						} else if (status == 1) {
							fileInfo.compilable = false;
						}

						auto compileTime = consolePrinter.finishedJob(&project);
						if (not cached or fileInfo.compileTime < compileTime) {
							fileInfo.compileTime = compileTime;
						}
						return status;
					}
					if (not fileInfo.compilable) {
						return 1;
					}
					return 0;
				}
			});
		}

		runToolchain("end");
	}();
	std::cout << "done\n";

	saveFileCache(*cfgYamlCache);
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
	} catch (std::exception const& e) {
		std::cerr << "exception: " << busy::utils::exceptionToString(e, 0) << "\n";
		std::cerr << sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"});
	}
	return EXIT_FAILURE;
}
