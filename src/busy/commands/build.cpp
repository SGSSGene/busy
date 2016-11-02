#include "commands.h"

#include "CompileBatch.h"
#include "git.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <locale>
#include <process/Process.h>
#include <serializer/serializer.h>
#include "Build.h"


namespace {
	std::string workspaceFile { ".busy/workspace.bin" };
}

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;


namespace {

void cloningExtRepositories(busyConfig::PackageURL url) {
	utils::mkdir(".busy/tmp");
	std::string repoName = std::string(".busy/tmp/repo_") + url.name + ".git";
	if (utils::fileExists(repoName)) {
		utils::rm(repoName, true, true);
	}
	std::cout << "cloning " << url.url << std::endl;
	git::clone(".", url.url, url.branch, repoName);
	auto configPackage = busyConfig::readPackage(repoName);
	utils::mv(repoName, std::string("extRepositories/") + url.name);
}

void checkMissingDependencies() {

	std::queue<busyConfig::PackageURL> queue;

	// read config file, set all other data
	auto configPackage = busyConfig::readPackage(".");
	for (auto x : configPackage.extRepositories) {
		queue.push(x);
	}

	if (not utils::fileExists("extRepositories")) {
		utils::mkdir("extRepositories");
	}

	while (not queue.empty()) {
		auto element = queue.front();
		queue.pop();

		if (not utils::fileExists("extRepositories/" + element.name + "/busy.yaml")) {
			cloningExtRepositories(element);
		}
		auto config = busyConfig::readPackage("extRepositories/" + element.name);
		for (auto x : config.extRepositories) {
			queue.push(x);
		}
	}
}

}

namespace commands {

using namespace busy::commands;

bool build(std::string const& _rootProjectName, bool verbose, bool noconsole, int jobs) {
	std::string rootProjectName = "";
	if (_rootProjectName != "true") {
		rootProjectName = _rootProjectName;
	}

	checkMissingDependencies();

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	Workspace ws;

	auto const toolchainName = ws.getSelectedToolchain();
	auto const buildModeName = ws.getSelectedBuildMode();

	auto const ignoreProjects = ws.getExcludedProjects(toolchainName);

	std::string const buildPath = ".busy/" + toolchainName + "/" + buildModeName;
	std::string const outPath   = "build/" + toolchainName + "/" + buildModeName;

	std::cout << "Using buildMode: " << buildModeName << std::endl;
	std::cout << "Using toolchain: " << toolchainName << std::endl;

	auto const toolchain = ws.getToolchains().at(toolchainName);

	Visitor visitor(ws, rootProjectName);

	detail::Build build(ws, buildPath, outPath, rootProjectName, not noconsole);

	bool& errorDetected    = build.errorDetected;
	std::mutex& printMutex = build.printMutex;



	// create all needed path
	build.createAllNeededPaths();

	// create version files
	build.createVersionFiles();

	CompileBatch compileBatch(errorDetected, printMutex, buildPath, outPath, buildModeName, verbose, toolchain, ignoreProjects);
	compileBatch.mRPaths = ws.getRPaths();




	visitor.setStatisticUpdateCallback(build.getStatisticUpdateCallback());
	visitor.setCppVisitor([&] (Project const* _project, std::string const& _file) {
		compileBatch.compileCpp(_project, _file);
	});

	visitor.setCVisitor([&] (Project const* _project, std::string const& _file) {
		compileBatch.compileC(_project, _file);
	});

	visitor.setProjectVisitor([&] (Project const* _project) {
		if (errorDetected) return;
		switch (_project->getType()) {
		case Project::Type::StaticLibrary:
			compileBatch.linkStaticLibrary(_project);
			break;
		case Project::Type::SharedLibrary:
			compileBatch.linkSharedLibrary(_project);
			break;
		case Project::Type::Executable:
			compileBatch.linkExecutable(_project);
			break;
		}
	});

	visitor.visit(jobs);

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);



	if (errorDetected) {
		std::cout<<std::endl<< TERM_RED "Build failed" TERM_RESET;
	} else {
		std::cout<<std::endl<< TERM_GREEN "Build \033[32msucceeded" TERM_RESET;
	}

	std::cout<< " after " << time_span.count() << " seconds." << std::endl;
	return not errorDetected;
}
}
