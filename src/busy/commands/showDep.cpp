#include "commands.h"

#include "Workspace.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <serializer/serializer.h>
#include <mutex>

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void showDep(std::string const& _rootProjectName) {
	std::string rootProjectName = "";
	if (_rootProjectName != "true") {
		rootProjectName = _rootProjectName;
	}
	Workspace ws;

	auto const toolchainName = ws.getSelectedToolchain();
//	auto const buildModeName = ws.getSelectedBuildMode();

	auto const ignoreProjects = ws.getExcludedProjects(toolchainName);

/*	std::string const buildPath = ".busy/" + toolchainName + "/" + buildModeName;
	std::string const outPath   = "build/" + toolchainName + "/" + buildModeName;*/

/*	std::cout << "Using buildMode: " << buildModeName << std::endl;
	std::cout << "Using toolchain: " << toolchainName << std::endl;*/

//	auto const toolchain = ws.getToolchains().at(toolchainName);

//	ws.get
	Visitor visitor(ws, rootProjectName);

//	bool errorDetected {false};
//	std::mutex printMutex;



/*	// create all needed path
	build.createAllNeededPaths();

	// create version files
	build.createVersionFiles();

	CompileBatch compileBatch(errorDetected, printMutex, buildPath, outPath, buildModeName, verbose, toolchain, ignoreProjects);*/


//	visitor.setStatisticUpdateCallback(build.getStatisticUpdateCallback());
	visitor.setCppVisitor([&] (Project const* /*_project*/, std::string const& /*_file*/) {});
	visitor.setCVisitor([&] (Project const* /*_project*/, std::string const& /*_file*/) {});


	visitor.setProjectVisitor([&] (Project const* _project) {
		if (_project->getIsSingleFileProjects()) return;
		auto printDep = [&] () {
			for (auto p : _project->getDependenciesOnlyShared()) {
				std::cout << u8R"( D→  )" << p->getFullName() << std::endl;
			}
			for (auto p : _project->getDependenciesRecursiveOnlyStaticNotOverShared()) {
				std::cout << u8R"( →S→ )" << p->getFullName() << std::endl;
			}
		};
		switch (_project->getType()) {
		case Project::Type::StaticLibrary:
			std::cout << "static: " << _project->getFullName() << std::endl;
			break;
		case Project::Type::SharedLibrary:
			std::cout << "shared: " << _project->getFullName() << std::endl;
			printDep();
			break;
		case Project::Type::Plugin:
			std::cout << "plugin: " << _project->getFullName() << std::endl;
			printDep();
			break;
		case Project::Type::Executable:
			std::cout << "executable: " << _project->getFullName() << std::endl;
			printDep();
			break;
		}
	});

	visitor.visit(1);

/*	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);



	if (errorDetected) {
		std::cout<<std::endl<< TERM_RED "Build failed" TERM_RESET;
	} else {
		std::cout<<std::endl<< TERM_GREEN "Build \033[32msucceeded" TERM_RESET;
	}

	std::cout<< " after " << time_span.count() << " seconds." << std::endl;
	return not errorDetected;*/
}

}

