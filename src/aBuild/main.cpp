#include "Workspace.h"

#include <memory>
#include <stdio.h>
#include <future>
#include "BuildActionClang.h"

#include "utils.h"
#include "Process.h"
#include "git.h"
#include "estd.h"

using namespace aBuild;

static std::map<std::string, Toolchain> getAllToolchains(Workspace const& ws) {
	std::map<std::string, Toolchain> retList;

	std::map<std::string, Toolchain> searchPaths {
	     {"/usr/bin/gcc-4.9", {"system-gcc-4.9", true, "gcc-4.9", "g++-4.9", "ar"}},
	     {"/usr/bin/gcc-4.8", {"system-gcc-4.8", true, "gcc-4.8", "g++-4.8", "ar"}},
	     {"/usr/bin/gcc-4.7", {"system-gcc-4.7", true, "gcc-4.7", "g++-4.7", "ar"}},
	     {"/usr/bin/clang",   {"system-clang",   true, "clang",   "clang++", "ar"}},
	     };

	for (auto const& p : searchPaths) {
		if (utils::fileExists(p.first)) {
			retList[p.second.getName()] = p.second;
		}
	}

	for (auto const& package : ws.getAllValidPackages(true)) {
		for (auto const& tc : package.getToolchains()) {
			retList[tc.getName()] = tc;
		}
	}
	return retList;
}
static std::map<std::string, Installation> getAllInstallations(Workspace const& ws) {
	std::map<std::string, Installation> retList;
	for (auto const& package : ws.getAllValidPackages(true)) {
		for (auto const& in : package.getInstallations()) {
			retList[in.getName()] = in;
		}
	}
	return retList;
}


static void checkingMissingPackages(Workspace& ws) {
	// Checking missing packages
	auto missingPackages = ws.getAllMissingPackages();
	while (not missingPackages.empty()) {
		utils::runParallel<PackageURL>(missingPackages, [](PackageURL const& url) {
			utils::mkdir(".aBuild/tmp");
			std::string repoName = std::string(".aBuild/tmp/repo_") + url.getName() + ".git";
			utils::rm(repoName, true, true);
			git::clone(url.getURL(), url.getBranch(), repoName);
			Package package(url);
			jsonSerializer::read(repoName + "/aBuild.json", package);
			utils::mv(repoName, std::string("packages/") + package.getName());
		});
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


static void actionDefault(bool verbose = false) {
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

	auto linkingLibFunc     = action->getLinkingLibFunc();
	auto linkingExecFunc    = action->getLinkingExecFunc();
	auto compileFileCppFunc = action->getCompileCppFileFunc();
	auto compileFileCFunc   = action->getCompileCFileFunc();

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
static void actionTest() {
	if (utils::dirExists("./build/tests/")) {
		auto allTests = utils::listFiles("./build/tests/");
		std::cout<<"===Start testing==="<<std::endl;
		for (auto const& t : allTests) {
			auto p = std::string("./build/tests/")+t;
			system(p.c_str());
			std::cout<<" • running "<<p<<std::endl;
		}
	}
	for (auto const& d : utils::listDirs("build", true)) {
		if (d == "tests") continue;
		std::string path = std::string("./build/") + d + "/tests/";
		if (not utils::dirExists(path)) continue;
		for (auto const& t : utils::listFiles(path)) {
			auto p = path+t;
			system(p.c_str());
			std::cout<<" • running "<<p<<std::endl;
		}
	}
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
	Workspace ws(".");
	auto flavor    = ws.accessConfigFile().getFlavor();
	auto toolchain = ws.accessConfigFile().getToolchain();
	auto buildPath = "./build/" + toolchain + "/" + flavor + "/";

	auto allFiles = utils::listFiles(buildPath);

	for (auto const& f : allFiles) {
		std::cout<<"installing "<<f<<"; Error Code: ";
		auto oldFile = buildPath+f;
		auto newFile = std::string("/usr/bin/")+f;
		auto cpFile  = std::string("cp ")+newFile+" "+oldFile;
		auto error = rename(oldFile.c_str(), newFile.c_str());
		std::cout<<error<<std::endl;
		system(cpFile.c_str());
	}
}
static void actionStatus(std::string _flavor = "") {
	Workspace ws(".");
	if (_flavor != "") {
		if (_flavor == "release" || _flavor == "debug") {
			ws.accessConfigFile().setFlavor(_flavor);
		} else {
			throw "only \"release\" and \"debug\" are valid flavor arguments";
		}
	}
	std::cout << "current flavor:    " << ws.accessConfigFile().getFlavor() << std::endl;
	std::cout << "current toolchain: " << ws.accessConfigFile().getToolchain() << std::endl;
}

static void actionQuickFix() {
	if (not utils::fileExists("aBuild.json")) {
		auto dir = utils::explode(utils::cwd(), "/");
		std::string packageName = dir[dir.size()-1];

		Package package {PackageURL()};
		package.setName(packageName);

		auto projectDirs = utils::listDirs("src", true);
		for (auto const& d : projectDirs) {
			Project p;
			p.set(d, "executable");
			package.accessProjects().push_back(std::move(p));
		}
		jsonSerializer::write("aBuild.json", package);
	}
}

std::string getLsFilesString(std::string const& path) {
	auto printLsFiles = std::string("bash -c \"cat <(git ls-files -o --exclude-standard) <(git ls-files) | sed 's/^/") +path+ std::string("\\//'\"");
	return printLsFiles;
}
static int actionListFiles() {
	if (not utils::fileExists("aBuild.json")) return EXIT_FAILURE;
	auto printLsFiles = getLsFilesString(".");
	system(printLsFiles.c_str());
	auto projectDirs = utils::listDirs("packages", true);
	for (auto const& d : projectDirs) {
		auto path = std::string("packages/")+d;
		utils::Cwd cwd(path);
		auto printLsFiles = getLsFilesString(std::string("packages\\/") + d);
		system(printLsFiles.c_str());
	}

	return EXIT_SUCCESS;
}
static void actionToolchains() {
	Workspace ws(".");
	auto toolchains    = getAllToolchains(ws);
	auto installations = getAllInstallations(ws);

	int longestString = 0;
	for (auto const& e : toolchains) {
		longestString = std::max(longestString, int(e.second.getName().length()));
	}

	for (auto const& e : toolchains) {
		auto const& tc = e.second;
		std::cout << tc.getName();
		for (int i (tc.getName().length()); i < longestString; ++i) {
			std::cout << " ";
		}
		bool installed = tc.isInstalled(installations);
		if (installed) {
			std::cout << " (installed";
		} else {
			std::cout << " (not installed";
		}
		if (tc.isAutoGenerated()) {
			std::cout << "/auto generated";
		}
		std::cout << ")" << std::endl;
	}
}
static void actionToolchain(std::string const& _toolchain) {
	Workspace ws(".");
	auto toolchains = getAllToolchains(ws);
	auto installations = getAllInstallations(ws);

	if (toolchains.find(_toolchain) != toolchains.end()) {
		if (not toolchains.at(_toolchain).isInstalled(installations)) {
			std::cout << "Toolchain is not installed." << std::endl;
			std::cout << "trying to install..." << std::endl;
			toolchains.at(_toolchain).install(installations);
			if (not toolchains.at(_toolchain).isInstalled(installations)) {
				return;
			}

		}
		ws.accessConfigFile().setToolchain(_toolchain);
		std::cout << "Set toolchain to " << _toolchain << std::endl;
		return;
	}
	std::cout << "Setting toolchain failed" << std::endl;
}

using Action = std::function<void()>;

int main(int argc, char** argv) {
	std::string arg1;
	if (argc >= 2) {
		arg1 = std::string(argv[1]);
	}
	try {
		if (argc == 1) {
			actionDefault();
		} else if (arg1 == "--verbose") {
			actionDefault(true);
		} else if (arg1 == "test") {
			actionDefault();
			actionTest();
		} else if (argc == 4 && arg1 == "clone") {
			actionClone(std::string(argv[2]), std::string(argv[3]) + "/");
		} else if (argc == 3 && arg1 == "clone") {
			std::string url  = argv[2];
			std::string path = argv[2];
			if (utils::isEndingWith(path, ".git")) {
				for (int i {0}; i<4; ++i) path.pop_back();
			}
			auto l = utils::explode(path, "/");
			path = l[l.size()-1];
			actionClone(url, path + "/");

		} else if (arg1 == "pull") {
			actionPull();
		} else if (arg1 == "push") {
			actionPush();
		} else if (arg1 == "install") {
			actionDefault();
			actionInstall();
		} else if (arg1 == "quickfix" or arg1 == "qf") {
			actionQuickFix();
		} else if (arg1 == "status") {
			actionStatus();
		} else if (argc == 3 && arg1 == "--flavor") {
			actionStatus(argv[2]);
		} else if (arg1 == "ls-files") {
			return actionListFiles();
		} else if (arg1 == "toolchains") {
			actionToolchains();
		} else if (argc == 3 && arg1 == "toolchain") {
			actionToolchain(std::string(argv[2]));
		} else {
			throw std::string("unknown command: ") + arg1;
		}
	} catch(std::exception const& e) {
		std::cerr<<"exception(exception): "<<e.what()<<std::endl;
	} catch(std::string const& s) {
		std::cerr<<"exception(string): "<<s<<std::endl;
	} catch(char const* s) {
		std::cerr<<"exception(char): "<<s<<std::endl;
	}
	return 0;
}
