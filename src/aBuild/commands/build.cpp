#include "commands.h"

#include <chrono>
#include <iostream>
#include <fstream>

#include <serializer/serializer.h>

#include "BuildAction.h"
#include "FileStates.h"

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"


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
	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

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
	auto linkingExecFunc        = action->getLinkingExecFunc(false);
	auto _linkingSLibFunc       = action->getLinkingExecFunc(true);
	auto _compileFileCppFunc    = action->getCompileFunc("c++11");
	auto _compileFileCFunc      = action->getCompileFunc("c11");

	std::function<bool(Project*)> linkingSLibFunc = [=] (Project* p) {
		bool success = linkingLibFunc(p);
		if (not success) return false;
		return _linkingSLibFunc(p);
	};

	std::function<bool(std::string*)> compileFileCppFunc = [=] (std::string* p){
		bool success = _compileFileCppFunc(p);
		return success;
	};
	std::function<bool(std::string*)> compileFileCFunc = [=] (std::string* p) {
		bool success = _compileFileCFunc(p);
		return success;
	};

	auto excludedProjects = ws.getExcludedProjects();
	auto requiredProjects = ws.getAllRequiredProjects();

	auto packagesDir = utils::listDirs("extRepositories", true);
	for (auto& p : packagesDir) {
		p = "extRepositories/" + p;
	}
	packagesDir.push_back(".");


	// find all modules that are needed as shared
	std::set<std::string> projectsNeededAsShared;
	for (auto& e  : requiredProjects) {
		// Check which projects are needed as shared libraries
		for (auto const& l : e.second.getLinkAsShared()) {
			projectsNeededAsShared.insert(l);
		}
	}

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

	// Read root Package name
	auto rootPackageName = ws.getRootPackageName();

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
			auto path = project.getPackagePath();
			if (path.size() > 1) {
				path.erase(path.begin(), path.begin()+9);
			} else {
				path = rootPackageName;
			}
			auto name = path + "/" + project.getName();
			if (projectsNeededAsShared.count(name) > 0) {
				graph.addNode(&project, linkingSLibFunc);
			} else {
				graph.addNode(&project, linkingLibFunc);
			}


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
			auto depName = dep;
			if (dep.front() == '.') {
				depName = rootPackageName + dep.substr(1);
			}
			auto const& las = project.getLinkAsShared();
			if (std::find(las.begin(), las.end(), depName) == las.end()) {
				graph.addEdge(&projects.at(key), &project);
			} else {
				graph.addEdge(&projects.at(key), &project);
//				std::cout << "not added static link to...." << depName << std::endl;
			}
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

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

	if (not success) {
		std::cout<<std::endl<< TERM_RED "Build failed" TERM_RESET;
		std::cout<< " after " << time_span.count() << " seconds." << std::endl;
		return false;
	} else {
		std::cout<<std::endl<< TERM_GREEN "Build \033[32msucceeded" TERM_RESET;
		std::cout << " after " << time_span.count() << " seconds." << std::endl;
		ws.accessConfigFile().setLastCompileTime(timeSinceBegin.count());
		ws.save();
	}
	return true;
}

}

