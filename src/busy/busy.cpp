#include "Package.h"
#include "Queue.h"
#include "utils/utils.h"
#include "cache.h"
#include "config.h"
#include "toolchains.h"
#include "CompilePipe.h"
#include "overloaded.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <process/Process.h>
#include <sargparse/ArgumentParsing.h>
#include <sargparse/Parameter.h>
#include <yaml-cpp/yaml.h>
#include <thread>

namespace busy {
namespace analyse {

auto cfgVerbose = sargp::Flag{"verbose", "verbose output"};

template <typename>
class nothing;


// check if this _project is included by _allIncludes
auto isDependentProject(std::set<std::filesystem::path> const& _allIncludes, busy::analyse::Project const& _project) -> bool {

	auto files = _project.getFiles();
	for (auto const& file : files) {
		// check if is includable by default path
		{
			auto path = _project.getName() / relative(file.getPath(), _project.getPath());
			if (_allIncludes.count(path)) {
				return true;
			}
		}
		// check if it is includable by legacy include path
		{
			for (auto const& p : _project.getLegacyIncludePaths()) {
				auto path = relative(file.getPath(), p);
				if (_allIncludes.count(path)) {
					return true;
				}
			}
		}
	}
	return false;
}

auto findDependentProjects(busy::analyse::Project const& _project, std::vector<busy::analyse::Project> const& _projects) {
	auto ret = std::set<busy::analyse::Project const*>{};
	auto _allIncludes = _project.getIncludes();

	for (auto const& project : _projects) {
		if (isDependentProject(_allIncludes, project)) {
			ret.emplace(&project);
		}
	}
	return ret;
}

auto createProjects(std::vector<busy::analyse::Project> const& _projects) -> ProjectMap {
	using Project = busy::analyse::Project;

	auto ret = ProjectMap{};

	for (auto const& p : _projects) {
		ret[&p];
		auto deps = findDependentProjects(p, _projects);
		for (auto const& d : deps) {
			if (d == &p) continue;
			std::get<0>(ret[&p]).insert(d);
			std::get<1>(ret[d]).insert(&p);
		}
	}

	return ret;
}

void checkConsistency(std::vector<busy::analyse::Project> const& _projects) {
	auto groupedProjects = std::map<std::string, std::vector<busy::analyse::Project const*>>{};
	for (auto const& p : _projects) {
		groupedProjects[p.getName()].emplace_back(&p);
	}


	for (auto const& [name, list] : groupedProjects) {
		//!TODO this can be done much more efficient
		if (size(list) > 1) {
			for (auto const& p1 : list) {
				for (auto const& p2 : list) {
					if (p1 == p2) continue;
					if (not p1->isEquivalent(*p2)) {
						throw std::runtime_error("two projects don't seem to be the same: " + p1->getName());
					}
				}
			}
		}
	}
}

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
				std::cout << "    - " << d->getName() << " (" << d->getPath() << ")\n";
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

auto cfgRootPath  = sargp::Parameter<std::string>{"..", "root",  "path to directory containing busy.yaml"};
auto cfgBuildPath = sargp::Parameter<std::string>{".",  "build", "path to build directory"};

auto cmdLsToolchains = sargp::Command{"ls-toolchains", "list all available toolchains", []() {
	auto config = [&]() {
		if (exists(global_busyConfigFile)) {
			return fon::yaml::deserialize<Config>(YAML::LoadFile(global_busyConfigFile));
		}
		return Config{};
	}();

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

auto cfgOptions   = sargp::Parameter<std::vector<std::string>>{{}, "option", "options for toolchains"};
auto cfgToolchain = sargp::Parameter<std::string>{"", "toolchain", "set toolchain", [](){}, [](std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>> {
	auto ret = std::pair<bool, std::set<std::string>>{false, {}};

	auto config = [&]() {
		if (exists(global_busyConfigFile)) {
			return fon::yaml::deserialize<Config>(YAML::LoadFile(global_busyConfigFile));
		}
		return Config{};
	}();

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

	if (cfgBuildPath and relative(std::filesystem::path{*cfgBuildPath}) != ".") {
		std::filesystem::create_directories(*cfgBuildPath);
		std::filesystem::current_path(*cfgBuildPath);
		std::cout << "changing working directory to " << *cfgBuildPath << "\n";
	}

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

struct ConsoleTimer {
	std::mutex mutex;
	std::chrono::milliseconds totalTime{0};
	int jobs{0};
	int totalJobs{0};
	std::string currentJob = "init";
	std::condition_variable cv;

	std::atomic_bool isRunning{true};
	std::thread thread;
	ConsoleTimer(std::chrono::milliseconds _total, int _totalJobs)
	: totalTime {_total}
	, totalJobs {_totalJobs}
	, thread {[this]() {
		auto startTime = std::chrono::steady_clock::now();
		auto nextTime = startTime;
		while(isRunning) {
			auto g = std::unique_lock{mutex};
			nextTime += std::chrono::milliseconds{1000};
			cv.wait_until(g, nextTime);

			auto now = std::chrono::steady_clock::now();
			auto diff = duration_cast<std::chrono::milliseconds>(now - startTime);
			std::cout << "Jobs " << jobs << "/" << totalJobs << " - ETA " << (diff.count() / 1000.) << "s/" << (totalTime.count() / 1000.) << "s - " << currentJob << "\n";
		}
	}}

	{}

	~ConsoleTimer() {
		isRunning = false;
		cv.notify_one();
		thread.join();
	}
	void addTotalTime(std::chrono::milliseconds _total) {
		auto g = std::unique_lock{mutex};
		totalTime += _total;
	}
	void addJob(int _jobs) {
		auto g = std::unique_lock{mutex};
		jobs += _jobs;
	}
	void setCurrentJob(std::string _currentJob) {
		auto g = std::unique_lock{mutex};
		currentJob = std::move(_currentJob);
	}

};

void app() {
	auto workPath = std::filesystem::current_path();

	if (cfgBuildPath and relative(std::filesystem::path{*cfgBuildPath}) != ".") {
		std::filesystem::create_directories(*cfgBuildPath);
		std::filesystem::current_path(*cfgBuildPath);
		std::cout << "changing working directory to " << *cfgBuildPath << "\n";
	}

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
		std::cout << "setting toolchain to " << config.toolchain.name << " (" << config.toolchain.call << ")\n";

	}
	auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);
	{
		// initialize queue
		auto queue = std::queue<std::string>{};
		auto processed = std::set<std::string>{};
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
	}


	std::cout << "using toolchain " << config.toolchain.name << " (" << config.toolchain.call << ")\n";
	std::cout << "  with options: ";
	for (auto const& o : config.toolchain.options) {
		std::cout << o << " ";
	}
	std::cout << "\n";


//	for (auto const& p : projects) {
//		std::cout << p.getName() << " (" << p.getPath() << ")\n";
//	}

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


	auto addRootDir = [&](std::filesystem::path path) {
		if (path.is_relative()) {
			path = std::filesystem::relative(config.rootDir / path);
		}
		return path;
	};

	std::cout << "start compiling...\n";
	{
		auto recompilingFiles    = std::unordered_set<busy::analyse::File const*>{};
		auto recompilingProjects = std::unordered_set<busy::analyse::Project const*>{};

		// precompute timings
		auto total = std::chrono::milliseconds{0};
		{
			auto pipe = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};
			while (not pipe.empty()) {
				pipe.dispatch(overloaded {
					[&](busy::analyse::File const& file, auto const& params) {
						auto path = addRootDir(file.getPath());
						auto& fileInfo = getFileInfos().get(path);
						if (*cfgClean or (fileInfo.hasChanged() and fileInfo.compilable)) {
							recompilingFiles.insert(&file);
							total += fileInfo.compileTime;
						}
						return 0;
					}, [&](busy::analyse::Project const& project, auto const& params) {
						auto path = addRootDir(project.getPath());
						auto& fileInfo = getFileInfos().get(path);
						auto anyChanges = [&]() {
							for (auto const& file : project.getFiles()) {
								if (recompilingFiles.count(&file) > 0) {
									return true;
								}
							}
							return false;
						};
						if (*cfgClean or (anyChanges() and fileInfo.compilable)) {
							recompilingProjects.insert(&project);
							total += fileInfo.compileTime;
						}
						return 0;
					}
				});
			}
		}
		//std::cout << "Estimated Compile Time: " << total.count() / 1000. << "\n";
		auto consoleTimer = ConsoleTimer{total, recompilingFiles.size() + recompilingProjects.size()};

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

		auto totalCheckTime = std::chrono::milliseconds{0};
		auto startCheckTime = std::chrono::steady_clock::now();
		while (not pipe.empty()) {
			pipe.dispatch(overloaded {
				[&](busy::analyse::File const& file, auto const& params) {
					auto startTimer = std::chrono::steady_clock::now();
					auto startTime  = std::chrono::file_clock::now();
					auto path = addRootDir(file.getPath());

					auto& fileInfo = getFileInfos().get(path);

					// check if file needs recompile
					auto status = [&]() {
						if (recompilingFiles.count(&file) > 0) {
							consoleTimer.setCurrentJob("compiling " + path.string());
//							std::cout << "compiling - " << path << "\n";

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
									path = addRootDir(path);
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


							consoleTimer.addTotalTime(-fileInfo.compileTime);
							auto stopTimer = std::chrono::steady_clock::now();
							total -= fileInfo.compileTime;
							totalCheckTime      += fileInfo.compileTime;
							auto compileTime = duration_cast<std::chrono::milliseconds>(stopTimer - startTimer);
							if (not cached or fileInfo.compileTime < compileTime) {
								fileInfo.compileTime = compileTime;
							}
							auto diff = duration_cast<std::chrono::milliseconds>(stopTimer - startCheckTime);
							consoleTimer.addTotalTime(+compileTime);
							consoleTimer.addJob(1);

							total = std::max(total + fileInfo.compileTime, totalCheckTime);
//							std::cout << "estimate " << totalCheckTime.count() / 1000. << "s (" << diff.count() / 1000. <<  "s)/" << total.count() / 1000. << "s\n";
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
				}, [&](busy::analyse::Project const& project, auto const& params) {
					auto path = addRootDir(project.getPath());
					auto& fileInfo = getFileInfos().get(path);

					if (recompilingProjects.count(&project) > 0) {
						auto startTimer = std::chrono::steady_clock::now();
//						std::cout << "linking: " << path << "\n";
						consoleTimer.setCurrentJob("linking " + path.string());
						auto [status, cout] = execute(params);
						bool cached = false;

						if (status == 0) {
							auto node = YAML::Load(cout);
							if (YAML::Node{node["cached"]}.as<bool>()) {
								cached = true;
							}

							consoleTimer.addTotalTime(-fileInfo.compileTime);
							auto stopTimer = std::chrono::steady_clock::now();
							total -= fileInfo.compileTime;
							totalCheckTime      += fileInfo.compileTime;
							auto compileTime = duration_cast<std::chrono::milliseconds>(stopTimer - startTimer);
							if (not cached or fileInfo.compileTime < compileTime) {
								fileInfo.compileTime = compileTime;
							}
							auto diff = duration_cast<std::chrono::milliseconds>(stopTimer - startCheckTime);
							consoleTimer.addTotalTime(+compileTime);
							total = std::max(total + fileInfo.compileTime, totalCheckTime);
//							std::cout << "estimate " << totalCheckTime.count() / 1000. << "s (" << diff.count() / 1000. <<  "s)/" << total.count() / 1000. << "s\n";
						} else if (status == 1) {
							fileInfo.compilable = false;
						}
						consoleTimer.addJob(1);
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
	}
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
