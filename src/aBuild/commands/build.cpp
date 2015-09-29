#include "commands.h"

using namespace aBuild;


namespace commands {

void build(bool verbose) {
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

	std::cout << "Using toolchain: " << toolchain.getName();
	std::cout << std::endl;

	std::unique_ptr<BuildAction> action { new BuildActionClang(&graph, verbose, &ws.accessConfigFile(), toolchain) };

	auto linkingLibFunc         = action->getLinkingLibFunc();
	auto linkingExecFunc        = action->getLinkingExecFunc();
	auto _compileFileCppFunc    = action->getCompileCppFileFunc();
	auto _compileFileCppFuncDep = action->getCompileCppFileFuncDep();
	auto compileFileCFunc       = action->getCompileCFileFunc();
	std::function<void(std::string*)> compileFileCppFunc     = [&] (std::string* p){
		_compileFileCppFunc(p);
		_compileFileCppFuncDep(p);
	};

	// Create dependency tree
	auto projects = ws.getAllRequiredProjects();
	for (auto& e  : projects) {
		auto& project = e.second;
		// Adding linking
		if (project.getType() == "library") {
			graph.addNode(&project, linkingLibFunc);
		} else if (project.getType() == "executable") {
			graph.addNode(&project,  linkingExecFunc);
		} else {
			std::cout<<"invalid type: "<<project.getType()<<std::endl;
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

	graph.visitAllNodes();
}

}

