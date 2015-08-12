#include "Workspace.h"

#include <memory>
#include <stdio.h>
#include <future>
#include "BuildAction.h"

#include "utils.h"
#include "git.h"
#include "estd.h"

using namespace aBuild;

static void checkingMissingPackages(Workspace& ws) {
	// Checking missing packages
	auto missingPackages = ws.getAllMissingPackages();
	while (not missingPackages.empty()) {
		for (auto const& p : missingPackages) {
			PackageURL url(p);
			utils::mkdir(".aBuild/tmp");
			utils::rm(".aBuild/tmp/Repo.git", true, true);
			git::clone(url.getURL(), url.getBranch(), std::string(".aBuild/tmp/Repo.git"));
			Package package(url);
			jsonSerializer::read(".aBuild/tmp/Repo.git/aBuild.json", package);
			std::string call = std::string("mv .aBuild/tmp/Repo.git packages/")+package.getName();
			system(call.c_str());
		}
		missingPackages = ws.getAllMissingPackages();
	}
}
static void checkingNotNeededPackages(Workspace& ws) {
	// Checking not needed packages
	auto notRequiredPackages     = ws.getAllNotRequiredPackages();
	for (auto const& s : notRequiredPackages) {
		std::cout<<"Found not required package "<<s<<std::endl;
	}
}
static void checkingInvalidPackages(Workspace& ws) {
	// checking invalid packages
	auto invalidPackages     = ws.getAllInvalidPackages();
	auto notRequiredPackages = ws.getAllNotRequiredPackages();
	for (auto const& p : invalidPackages) {
		if (estd::find(notRequiredPackages, p) == notRequiredPackages.end()) {
			std::cout<<"Package is ill formed: "<<p<<std::endl;
		}
	}
}

static void checkingRequiredPackages(Workspace& ws) {
	// check branches (if the correct ones are checked out
	auto requiredPackages = ws.getAllRequiredPackages();
	for (auto const& _url : requiredPackages) {
		PackageURL url {_url};
		utils::Cwd cwd(url.getPath());
		if (not git::isDirty()) {
			if (url.getBranch() != git::getBranch()) {
				std::cout<<"Changing branch of "<<url.getName()<<std::endl;
				git::checkout(url.getBranch());
			}
		}
	}
}


static void actionDefault() {
	Workspace ws(".");

	checkingMissingPackages(ws);
	checkingNotNeededPackages(ws);
	checkingInvalidPackages(ws);
	checkingRequiredPackages(ws);


	Graph graph;

	auto linkingLibFunc  = BuildAction::getLinkingLibFunc(graph);
	auto linkingExecFunc = BuildAction::getLinkingExecFunc(graph);
	auto compileFileFunc = BuildAction::getCompileFileFunc(graph);

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
			graph.addNode(&f, compileFileFunc);
			graph.addEdge(&f, &project);
		}
		for (auto const& dep : project.getDependencies()) {
			auto l = utils::explode(dep, "/");
			graph.addEdge(&projects.at(l[1]), &project);
		}
	}

	graph.visitAllNodes();
}
static void actionTest() {
	auto allTests = utils::listFiles("./build/tests/");
	std::cout<<"===Start testing==="<<std::endl;
	for (auto const& t : allTests) {
		auto p = std::string("./build/tests/")+t;
		system(p.c_str());
		std::cout<<" • running "<<p<<std::endl;
	}
	for (auto const& d : utils::listDirs("build")) {
		std::string path = std::string("./build/") + d + "/tests/";
		for (auto const& t : utils::listFiles(path)) {
			auto p = path+t;
			system(p.c_str());
			std::cout<<" • running "<<p<<std::endl;
		}
	}
	std::cout<<"ran "<<allTests.size()<<" test(s)"<<std::endl;
	std::cout<<"===Ended testing==="<<std::endl;
}
static void actionClone(std::string const& _url, std::string const& _dir) {
	git::clone(_url, "master", _dir);
	utils::Cwd cwd {_dir};
	actionDefault();
	actionTest();
}
static void actionPull() {
	auto allPackages = utils::listDirs("./packages", true);
	for (auto& p : allPackages) { p = "./packages/"+p; }
	allPackages.push_back(".");

	utils::runParallel<std::string>(allPackages, [](std::string const& file) {
		utils::Cwd cwd(file);
		if (git::isDirty()) {
			std::cout<<"ignore " << file << ": Dirty repository"<<std::endl;
		} else {
			git::pull();
		}
	});
}
static void actionPush() {
	auto allPackages = utils::listDirs("./packages", true);
	for (auto& p : allPackages) { p = "./packages/"+p; }
	allPackages.push_back(".");

	utils::runParallel<std::string>(allPackages, [](std::string const& file) {
		utils::Cwd cwd(file);
		git::push();
	});
}
static void actionInstall() {
	auto allFiles = utils::listFiles("./build/");
	for (auto const& f : allFiles) {
		std::cout<<"installing "<<f<<"; Error Code: ";
		auto oldFile = std::string("./build/")+f;
		auto newFile = std::string("/usr/bin/")+f;
		auto cpFile  = std::string("cp ")+newFile+" "+oldFile;
		auto error = rename(oldFile.c_str(), newFile.c_str());
		std::cout<<error<<std::endl;
		system(cpFile.c_str());
	}
}

using Action = std::function<void()>;

int main(int argc, char** argv) {
	try {
		if (argc == 1) {
			actionDefault();
		} else if (argc == 2 && std::string(argv[1]) == "test") {
			actionDefault();
			actionTest();
		} else if (argc == 4 && std::string(argv[1]) == "clone") {
			actionClone(std::string(argv[2]), std::string(argv[3]) + "/");
		} else if (argc == 2 && std::string(argv[1]) == "pull") {
			actionPull();
		} else if (argc == 2 && std::string(argv[1]) == "push") {
			actionPush();
		} else if (argc == 2 && std::string(argv[1]) == "install") {
			actionDefault();
			actionInstall();
		}


	} catch(std::exception const& e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}
	return 0;
}
