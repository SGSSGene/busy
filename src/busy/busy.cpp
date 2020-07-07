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
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>
#include <sargparse/ArgumentParsing.h>
#include <sargparse/File.h>
#include <sargparse/Parameter.h>
#include <yaml-cpp/yaml.h>

#include "cmd/compile.h"

namespace busy {

void listToolchains(std::vector<std::filesystem::path> const& packages) {
	for (auto [name, path]  : searchForToolchains(packages)) {
		fmt::print("  - {} ({})\n", name, path);
	}
}

auto cmdLsToolchains = sargp::Command{"ls-toolchains", "list all available toolchains", []() {
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});

	auto packages = std::vector<std::filesystem::path>{};
	if (!config.rootDir.empty() and config.rootDir != "." and std::filesystem::exists(config.rootDir)) {
		auto [pro, pack] = busy::readPackage(config.rootDir, ".");
		for (auto p : pack) {
			packages.emplace_back(p);
		}
	}

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	listToolchains(packages);
}};


auto cmdShowDeps = sargp::Command{"show-deps", "show dependencies of projects", []() {
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});

	auto [projects, packages] = busy::readPackage(config.rootDir, ".");

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	// check consistency of packages
	fmt::print("checking consistency...");
	checkConsistency(projects);
	fmt::print("done\n");

	auto projects_with_deps = createProjects(projects);
	printProjects(projects_with_deps);

}};

auto cmdVersionShow = []() {
	fmt::print("busy 2.0.0-git-alpha\n");
	fmt::print("Copyright (C) 2020 Simon Gene Gottlieb\n");
};
auto cmdVersion   = sargp::Flag{"version", "show version", cmdVersionShow};

auto cmdCleanCache = []() {
	auto allRemovedFiles = std::uintmax_t{};
	for (auto& p : std::filesystem::directory_iterator{"."}) {
		if (p.path() != "./.busy.yaml") {
			allRemovedFiles += std::filesystem::remove_all(p.path());
		}
	}
	fmt::print("cleaned busy caches - removed {} files\n", allRemovedFiles);
};
auto cmdClean     = sargp::Command{"clean", "cleans cache", cmdCleanCache};

auto cmdStatusShow = []() {
	fmt::print("print status");
};
auto cmdStatus    = sargp::Command("status", "show status", cmdStatusShow);

}

namespace {
	auto printHelp  = sargp::Parameter<std::optional<std::string>>{{}, "help", "print this help - add a string for  grep-like search"};
}

int main(int argc, char const** argv) {
	try {
		if (std::string_view{argv[argc-1]} == "--bash_completion") {
			auto hint = sargp::compgen(argc-2, argv+1);
			fmt::print("{}", hint);
			return 0;
		}

		sargp::parseArguments(argc-1, argv+1);
		if (printHelp) {
			fmt::print("{}", sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"}));
			return 0;
		}
		sargp::callCommands();
		return EXIT_SUCCESS;
	} catch (busy::CompileError const& e) {
	} catch (std::exception const& e) {
		fmt::print(std::cerr, "exception {}\n", busy::utils::exceptionToString(e, 0));
		fmt::print(std::cerr, "{}", sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"}));
	}
	return EXIT_FAILURE;
}
