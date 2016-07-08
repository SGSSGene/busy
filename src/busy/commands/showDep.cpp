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

void showDep(std::string const& rootProjectName) {
	std::vector<Project const*> rootProjects;

	Workspace ws(".");

	Graph graph;

	std::function<bool(Project*)>     linkingLibFunc      = [] (Project*) { return true; };
	std::function<bool(Project*)>     linkingExecFunc     = [] (Project*) { return true; };
	std::function<bool(Project*)>     _linkingSLibFunc    = [] (Project*) { return true; };
	std::function<bool(std::string*)> _compileFileCppFunc = [] (std::string*) { return true; };
	std::function<bool(std::string*)> _compileFileCFunc   = [] (std::string*) { return true; };
	std::function<bool(Project*)>     linkingSLibFunc     = [] (Project*) { return true; };
	std::function<bool(std::string*)> compileFileCppFunc  = [] (std::string*){ return true; };
	std::function<bool(std::string*)> compileFileCFunc    = [] (std::string*) { return true; };

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
				requiredProjects[d].setAutoDependenciesDiscovery(true);
			}

			auto& project = requiredProjects.at(d);
			if (project.getIgnore()) continue;
			if (not project.getAutoDependenciesDiscovery()) continue;

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
	std::cout << "    linkAsShared:" << std::endl;
	if (projects.find(rootProjectName) != projects.end()) {
		auto ingoing = graph.getIngoing<Project, Project>(&projects.at(rootProjectName), false);
		for (auto const& p : ingoing) {
			auto path = utils::explode(p->getPackagePath(), "/");
			std::string packageName = rootPackageName;
			if (path.size() > 1) {
				packageName = path[1];
			}
			std::cout << "      - " << packageName << "/" << p->getName() << std::endl;
		}
	}
}

}

