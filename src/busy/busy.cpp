#include "Package.h"
#include "Queue.h"
#include "utils/utils.h"
#include "cache.h"
#include "config.h"
#include "toolchains.h"
#include "CompilePipe.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <process/Process.h>
#include <sargparse/ArgumentParsing.h>
#include <sargparse/Parameter.h>

namespace busy {
namespace analyse {

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
auto cfgToolchain = sargp::Parameter<std::string>{"", "toolchain", "set toolchain"};


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

	for (auto o : *cfgOptions) {
		if (o.length() > 3 and o.substr(0, 3) == "no-") {
			auto s = o.substr(3);
			config.toolchain.options.erase(s);
		} else {
			config.toolchain.options.insert(o);
		}
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

	loadFileCache();

	if (cfgToolchain) {
		auto toolchains = searchForToolchains(packages);
		auto iter = toolchains.find(*cfgToolchain);
		if (iter == toolchains.end()) {
			throw std::runtime_error("could not find toolchain \"" + config.toolchain.name + "\"");
		}

		config.toolchain.call = iter->first;
		config.toolchain.call = iter->second;
		std::cout << "setting toolchain to " << config.toolchain.name << " (" << config.toolchain.call << ")\n";

	}
	auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);
	// copying config.toolchain.options for save erasure
	for (auto const& o : std::set<std::string>{config.toolchain.options}) {
		if (toolchainOptions.count(o) == 0) {
			std::cout << "unknown toolchain option " << o << " (removed)\n";
			config.toolchain.options.erase(o);
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
	std::cout << "checking consistency...";
	checkConsistency(projects);
	std::cout << "done\n";

	auto projects_with_deps = createProjects(projects);
	//printProjects(projects_with_deps);

	std::cout << "start compiling...";
	{
		auto pipe = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};

		auto runToolchain = [&](std::string_view str) {
			auto params = std::vector<std::string>{config.toolchain.call, std::string{str}};
			auto p = process::Process{params};

			if (p.getStatus() != 0) {
				throw std::runtime_error{"failed running toolchain " + std::string{str}};
			}
		};

		runToolchain("begin");

		while (not pipe.empty()) {
			pipe.dispatch([&](auto const& params) {
				auto p = process::Process{params};
/*				std::cout << "call:";
				for (auto p : params) {
					std::cout << " " << p;
				}
				std::cout << "\n";*/
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
					return 1;
				}
				return 0;
			});
		}

		runToolchain("end");
	}
	std::cout << "done\n";


	// Save config
	{
		YAML::Emitter out;
		out << fon::yaml::serialize(config);
		std::ofstream(global_busyConfigFile) << out.c_str();
	}

	saveFileCache();
}

auto cmdCompile = sargp::Command{"compile", "compile everything (default)", []() {
	app();
}};
auto cmdCompileDefault = sargp::Task{[]{
	if (cmdLsToolchains) return;
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
			std::cout << hint << " ";
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
