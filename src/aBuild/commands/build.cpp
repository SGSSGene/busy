#include "commands.h"

#include <chrono>
#include <iostream>

#include "BuildAction.h"
#include "FileStates.h"

using namespace aBuild;

namespace commands {

bool build(std::string const& rootProjectName, bool verbose, bool noconsole, int jobs) {
	std::vector<Project const*> rootProjects;

	Workspace ws(".");

	checkingMissingPackages(ws);
	//checkingNotNeededPackages(ws);
	checkingInvalidPackages(ws);
	checkingRequiredPackages(ws);


	Graph graph;

	auto allToolchains = getAllToolchains(ws);
	auto allFlavors    = getAllFlavors(ws);

	Toolchain toolchain = allToolchains.rbegin()->second;
	std::string toolchainName = ws.accessConfigFile().getToolchain();
	std::string lastFlavor    = ws.accessConfigFile().getLastFlavor();

	std::string buildMode = ws.accessConfigFile().getBuildMode();

	if (allToolchains.find(toolchainName) != allToolchains.end()) {
		toolchain = allToolchains.at(toolchainName);
	}
	// check if flavor has toolchain
	if (allFlavors.find(lastFlavor) != allFlavors.end()) {
		auto flavor = allFlavors.at(lastFlavor);
		if (allToolchains.find(flavor.toolchain) != allToolchains.end()) {
			if (flavor.buildMode == "release" || flavor.buildMode == "debug") {
				buildMode = flavor.buildMode;
				toolchain = allToolchains.at(flavor.toolchain);
			}
		}
	}
	ws.accessConfigFile().setToolchain(toolchain.getName());
	ws.accessConfigFile().setBuildMode(buildMode);
	ws.save();

	auto timeSinceBegin = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());


	std::cout << "Using buildMode: " << ws.accessConfigFile().getBuildMode() << std::endl;
	std::cout << "Using toolchain: " << ws.accessConfigFile().getToolchain() << std::endl;

	std::unique_ptr<BuildAction> action { new BuildAction(&graph, verbose, &ws.accessConfigFile(), toolchain) };

	auto linkingLibFunc         = action->getLinkingLibFunc();
	auto linkingExecFunc        = action->getLinkingExecFunc();
	auto _compileFileCppFunc    = action->getCompileCppFileFunc();
	auto _compileFileCppFuncDep = action->getCompileCppFileFuncDep();
	auto _compileFileCFunc      = action->getCompileCFileFunc();
	auto _compileFileCFuncDep   = action->getCompileCFileFuncDep();

	std::function<bool(std::string*)> compileFileCppFunc = [&] (std::string* p){
		bool error = _compileFileCppFunc(p);
		_compileFileCppFuncDep(p);
		return error;
	};
	std::function<bool(std::string*)> compileFileCFunc = [&] (std::string* p) {
		bool error = _compileFileCFunc(p);
		_compileFileCFuncDep(p);
		return error;
	};

	auto excludedProjects = ws.getExcludedProjects();
	auto requiredProjects = ws.getAllRequiredProjects();

	auto packagesDir = utils::listDirs("packages", true);
	for (auto& p : packagesDir) {
		p = "packages/" + p;
	}
	packagesDir.push_back(".");

	std::vector<Project*> autoProjects;

	for (auto const& pDir : packagesDir) {
		if (not utils::fileExists(pDir + "/src")) continue;
		// Find auto projects
		auto projectDirs = utils::listDirs(pDir + "/src", true);
		for (auto const& d : projectDirs) {
			if (excludedProjects.count(d) != 0) continue;
			auto iter = requiredProjects.find(d);
			if (iter == requiredProjects.end()) {
				requiredProjects[d].set(d);
				requiredProjects[d].setPackagePath(pDir);
				requiredProjects[d].setAuto(true);
			}

			auto& project = requiredProjects.at(d);
			if (project.getIgnore()) continue;
			if (not project.getAuto()) continue;

			autoProjects.push_back(&project);
		}
	}
	for (auto p : autoProjects) {
		auto& project = *p;

		auto dep    = project.getDefaultDependencies(&ws, requiredProjects);
		auto optDep = project.getDefaultOptionalDependencies(&ws, requiredProjects);

		for (auto d : optDep) {
			auto iter = std::find(dep.begin(), dep.end(), d);
			while (iter != dep.end()) {
				dep.erase(iter);
				iter = std::find(dep.begin(), dep.end(), d);
			}
		}

		project.setDependencies(std::move(dep));
		project.setOptionalDependencies(std::move(optDep));
	}
	// save all the auto detected dependencies
	ws.save();

	// Create dependency tree
	auto projects = requiredProjects;
	for (auto& e  : projects) {
		auto& project = e.second;
		if (project.getIgnore()) continue;
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
		// adding dependencies between projects
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
		// adding dependencies between optional projects
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
		return false;
	} else {
		std::cout<<"Build succeeded"<<std::endl;
		ws.accessConfigFile().setLastCompileTime(timeSinceBegin.count());
		ws.save();
	}
	return true;
}

}

