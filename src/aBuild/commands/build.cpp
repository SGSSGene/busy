#include "commands.h"

#include <chrono>
#include <iostream>

#include "BuildAction.h"

using namespace aBuild;

namespace commands {

void build(std::string const& rootProjectName, bool verbose, bool noconsole) {
	Project*    rootProject { nullptr };

	Workspace ws(".");

	checkingMissingPackages(ws);
	checkingNotNeededPackages(ws);
	checkingInvalidPackages(ws);
	checkingRequiredPackages(ws);


	Graph graph;

	auto allToolchains = getAllToolchains(ws);

	Toolchain toolchain = allToolchains.rbegin()->second;
	std::string toolchainName = ws.accessConfigFile().getToolchain();
	if (allToolchains.find(toolchainName) != allToolchains.end()) {
		toolchain = allToolchains.at(toolchainName);
	}
	ws.accessConfigFile().setToolchain(toolchain.getName());
	ws.save();

	{
		auto now = std::chrono::system_clock::now();
		auto timeSinceBegin = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
		ws.accessConfigFile().setLastCompileTime(timeSinceBegin.count());
	}

	std::cout << "Using flavor:    " << ws.accessConfigFile().getFlavor() << std::endl;
	std::cout << "Using toolchain: " << ws.accessConfigFile().getToolchain() << std::endl;

	std::unique_ptr<BuildAction> action { new BuildAction(&graph, verbose, &ws.accessConfigFile(), toolchain) };

	auto linkingLibFunc         = action->getLinkingLibFunc();
	auto linkingExecFunc        = action->getLinkingExecFunc();
	auto _compileFileCppFunc    = action->getCompileCppFileFunc();
	auto _compileFileCppFuncDep = action->getCompileCppFileFuncDep();
	auto compileFileCFunc       = action->getCompileCFileFunc();
	std::function<bool(std::string*)> compileFileCppFunc     = [&] (std::string* p){
		bool error = _compileFileCppFunc(p);
		_compileFileCppFuncDep(p);
		return error;
	};


	// Create dependency tree
	auto projects = ws.getAllRequiredProjects();
	for (auto& e  : projects) {
		auto& project = e.second;
		if (project.getName() == rootProjectName) {
			rootProject = &project;
		}
		// Adding linking
		if (project.getType() == "library") {
			graph.addNode(&project, linkingLibFunc);
		} else if (project.getType() == "executable") {
			graph.addNode(&project,  linkingExecFunc);
		} else {
			throw std::runtime_error("Project " + project.getName() + " has unknown type " + project.getType());
		}


		// Adding compile files
		for (auto& f : project.getAllCppFiles()) {
			graph.addNode(&f, compileFileCppFunc);
			graph.addEdge(&f, &project);
		}
		// Adding compile files
		for (auto& f : project.getAllCFiles()) {
			graph.addNode(&f, compileFileCFunc);
			graph.addEdge(&f, &project);
		}
		for (auto const& dep : project.getDependencies()) {
			auto l   = utils::explode(dep, "/");
			if (l.size() != 2) {
				throw std::runtime_error("Project " + project.getName() + " has unknown dependency " + dep);
			}
			auto key = l[l.size() -1];
			if (projects.find(key) == projects.end()) {
				throw std::runtime_error("Project " + project.getName() + " has unknown dependency " + dep);
			}
			graph.addEdge(&projects.at(key), &project);
		}
		for (auto const& dep : project.getOptionalDependencies()) {
			auto l   = utils::explode(dep, "/");
			if (l.size() != 2) {
				throw std::runtime_error("Project " + project.getName() + " has unknown dependency " + dep);
			}
			auto key = l[l.size() -1];
			if (projects.find(key) != projects.end()) {
				graph.addEdge(&projects.at(key), &project);
			}
		}
	}


	if (rootProject) {
		std::cout << "Compiling project: " << rootProject->getName() << std::endl;
		// Removing stuff that is not a dependency on rootProject
		graph.removeUnreachableIngoing<Project>({rootProject});
	}

	bool success = graph.visitAllNodes(10, [=](int done, int total, int totaltotal) {
		if (not noconsole) {
			std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
			std::cout << "working on job: " << done << "/" << total << "/" << totaltotal << std::flush;

			if (done == total) {
				std::cout << std::endl;
			}
		} else if (done == total) {
			std::cout << "working on job: "<< done << "/" << total << std::endl;
		}
	});
	if (not success) {
		std::cout<<"Build failed"<<std::endl;
	} else {
		std::cout<<"Build succeeded"<<std::endl;
	}
}

}

