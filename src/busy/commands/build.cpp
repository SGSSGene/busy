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


namespace commands {

using namespace busy::commands;

bool build(std::string const& _rootProjectName, bool verbose, bool noconsole, int jobs, bool _dryRun, bool _strict) {
	std::string rootProjectName = "";
	if (_rootProjectName != "true") {
		rootProjectName = _rootProjectName;
	}

	auto startTime = std::chrono::steady_clock::now();
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

	CompileBatch compileBatch(errorDetected, printMutex, buildPath, outPath, buildModeName, verbose, toolchain, ignoreProjects, _dryRun, _strict, false);
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
		if (_project->getIsSingleFileProjects()) return;

		switch (_project->getType()) {
		case Project::Type::StaticLibrary:
			compileBatch.linkStaticLibrary(_project);
			break;
		case Project::Type::SharedLibrary:
			compileBatch.linkSharedLibrary(_project);
			break;
		case Project::Type::Plugin:
			compileBatch.linkPlugin(_project);
			break;
		case Project::Type::Executable:
			compileBatch.linkExecutable(_project);
			break;
		}
	});

	visitor.visit(jobs);

	auto endTime   = std::chrono::steady_clock::now();
	auto diff      = endTime - startTime;
	auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(diff);



	if (errorDetected) {
		std::cout<<"\n" TERM_RED "Build failed" TERM_RESET;
	} else {
		std::cout<<"\n" TERM_GREEN "Build succeeded" TERM_RESET;
	}
	std::cout << " after " << time_span.count() / 1000. << " seconds." << std::endl;
	return not errorDetected;
}
}
