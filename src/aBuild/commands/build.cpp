#include "commands.h"

#include <chrono>
#include <iostream>

#include "BuildAction.h"

using namespace aBuild;

namespace commands {

void build(std::string const& rootProjectName, bool verbose, bool noconsole, int jobs) {
	std::vector<Project const*> rootProjects;

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

	auto excludedProjects = ws.getExcludedProjects();
	// Create dependency tree
	auto projects = ws.getAllRequiredProjects();
	for (auto& e  : projects) {
		auto& project = e.second;
		if (excludedProjects.count(project.getName()) != 0) continue;

		if ((project.getName() == rootProjectName) || (rootProjectName == "" && excludedProjects.size() > 0)) {
			rootProjects.push_back(&project);
		}

		// Adding linking
		if (project.getType() == "library") {
			graph.addNode(&project, linkingLibFunc);
		} else if (project.getType() == "executable") {
			graph.addNode(&project,  linkingExecFunc);
			if (rootProjectName == "" && excludedProjects.size() > 0) {
				rootProjects.push_back(&project);
			}
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
				if (excludedProjects.count(key) == 0) {
					graph.addEdge(&projects.at(key), &project);
				}
			}
		}
	}


	if (rootProjects.size() > 0) {
		std::cout << "Compiling project(s): ";
		for (auto p : rootProjects) {
			std::cout << p->getName() << ", ";
		}
		std::cout << std::endl;
		// Removing stuff that is not a dependency on rootProject
		graph.removeUnreachableIngoing(rootProjects);
	}

	bool success = graph.visitAllNodes(jobs, [=](int done, int total, int totaltotal) {
		if (not noconsole) {
			std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
			std::cout << "working on job: " << done << "/" << total << "/" << totaltotal << std::flush;

			if (done == totaltotal) {
				std::cout << std::endl;
			}
		} else if (done == totaltotal) {
			std::cout << "working on job: "<< done << "/" << total << "/" << totaltotal << std::endl;
		}
	});
	if (not success) {
		std::cout<<"Build failed"<<std::endl;
	} else {
		std::cout<<"Build succeeded"<<std::endl;
	}
}

}

