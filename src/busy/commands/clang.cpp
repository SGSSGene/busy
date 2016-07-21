#include "commands.h"
#include <iostream>

#include "BuildAction.h"

using namespace busy;

namespace commands {

void clang() {

	Workspace ws(".");

	cloneMissingPackages(ws);
	//checkingNotNeededPackages(ws);
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

	std::unique_ptr<BuildAction> action { new BuildAction(&graph, false, &ws.accessConfigFile(), toolchain) };

	std::function<bool(Project*)>     linkingLibFunc   = [](Project*) {return true; };
	std::function<bool(Project*)>     linkingExecFunc  = [](Project*) {return true; };
	std::function<bool(std::string*)> compileFileCFunc = [](std::string*) {return true; };

	auto compileClangCompleteFunc = action->getCompileClangCompleteFunc();

	std::function<bool(std::string*)> compileFileCppFunc     = [&] (std::string* p){
		if (*p == "src/justclangcomplete.cpp") {
			compileClangCompleteFunc(p);
		}
		return true;
	};
	std::list<std::string> fakeFiles;

	auto requiredProjects = ws.getAllRequiredProjects();

	auto packagesDir = utils::listDirs("extRepositories", true);
	for (auto& p : packagesDir) {
		p = "extRepositories/" + p;
	}
	packagesDir.push_back(".");

	// find auto dependencies
	std::vector<Project*> autoProjects;
	for (auto const& pDir : packagesDir) {
		if (not utils::fileExists(pDir + "/src")) continue;
		// Find auto projects
		auto projectDirs = utils::listDirs(pDir + "/src", true);
		for (auto const& d : projectDirs) {
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
	// save all the auto detected dependencies
	ws.save();


	// Create dependency tree
	auto projects = requiredProjects;
	for (auto& e  : projects) {
		auto& project = e.second;
		if (project.getIgnore()) continue;

		// Adding linking
		if (project.getType() == "library") {
			graph.addNode(&project, linkingLibFunc);
		} else if (project.getType() == "executable") {
			graph.addNode(&project,  linkingExecFunc);
		} else {
			std::cout<<"invalid type: "<<project.getType()<<std::endl;
		}

		if (project.getPackagePath() == ".") {
			fakeFiles.push_back("src/justclangcomplete.cpp");

			graph.addNode(&fakeFiles.back(), compileFileCppFunc);
			graph.addEdge(&fakeFiles.back(), &project);
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
			auto key = l[l.size() -1];
			graph.addEdge(&projects.at(key), &project);
		}
		for (auto const& dep : project.getOptionalDependencies()) {
			auto l   = utils::explode(dep, "/");
			auto key = l[l.size() -1];
			if (projects.find(key) != projects.end()) {
				graph.addEdge(&projects.at(key), &project);
			}
		}
	}
	utils::rm(".clang_complete", false, true);
	graph.visitAllNodes(10);
}


}
