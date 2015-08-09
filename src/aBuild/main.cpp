#include "Workspace.h"

#include <memory>
#include <stdio.h>
#include <future>

#include "utils.h"
#include "git.h"
#include "estd.h"
#include "graph.h"

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

using Action = std::function<void()>;

int main(int argc, char** argv) {
	try {
		if (argc == 1) {
			Workspace ws(".");

			checkingMissingPackages(ws);
			checkingNotNeededPackages(ws);
			checkingInvalidPackages(ws);
			checkingRequiredPackages(ws);


			Graph graph;

			std::function<void(Project*)> linkingLibFunc = [&graph](Project* project) {
				utils::mkdir(".aBuild/lib/");
				std::string call = "ar rcs .aBuild/lib/"+project->getName()+".a";
				// Get file dependencies
				{
					auto ingoing = graph.getIngoing<std::string, Project>(project, false);
					for (auto const& f : ingoing) {
						call += " .aBuild/obj/"+ *f + ".o";
					}
				}
				utils::runProcess(call);
//				std::cout<<call<<std::endl;
			};
			std::function<void(Project*)> linkingExecFunc = [&graph](Project* project) {
				utils::mkdir("bin");
				utils::mkdir("bin/tests");
				std::string outFile = std::string("bin/")+project->getName();
				if (utils::isStartingWith(project->getName(), "test")) {
					outFile = std::string("bin/tests/")+project->getName();
				}
				std::string call = "ccache clang++ -o "+outFile;

				// Set all depLibraries libraries
				for (auto const& l : project->getDepLibraries()) {
					call += " -l"+l;
				}
				// Get file dependencies
				{
					auto ingoing = graph.getIngoing<std::string, Project>(project, false);
					for (auto const& f : ingoing) {
						call += " .aBuild/obj/"+ *f + ".o";
					}
				}
				// Get project dependencies
				{
					auto outgoing = graph.getIngoing<Project, Project>(project, true);
					for (auto const& f : outgoing) {
						call += " .aBuild/lib/"+f->getName()+".a";

						// Set all depLibraries libraries
						for (auto const& l : f->getDepLibraries()) {
							call += " -l"+l;
						}

					}
				}
//				std::cout<<call<<std::endl;
				utils::runProcess(call);

			};

			std::function<void(std::string*)> compileFileFunc = [&graph](std::string* f) {

				auto l = utils::explode(*f, "/");

				utils::mkdir(".aBuild/obj/" + utils::dirname(*f));
				std::string call = "ccache clang++ -Qunused-arguments -ggdb -O0 --std=c++11 "
				                   "-c " + *f + " "
				                   "-o .aBuild/obj/" + *f + ".o";

				// Get include dependencies
				{
					Project* project = *graph.getOutgoing<Project, std::string>(f, false).begin();
					for (auto const& i : project->getLegacy().includes) {
						call += " -I "+project->getPackagePath()+"/"+i;
					}
					call += " -I "+project->getPackagePath()+"/src/"+project->getPath();
					call += " -I "+project->getPackagePath()+"/src/";

					auto ingoing = graph.getIngoing<Project, Project>(project, true);
					for (auto const& f : ingoing) {
						call += " -isystem "+f->getPackagePath()+"/src";
						for (auto const& i : f->getLegacy().includes) {
							call += " -isystem "+f->getPackagePath()+"/"+i;
						}
					}
				}
				utils::runProcess(call);
//				std::cout<<call<<std::endl;
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
					graph.addNode(&f, compileFileFunc);
					graph.addEdge(&f, &project);
				}
				for (auto const& dep : project.getDependencies()) {
					auto l = utils::explode(dep, "/");
					graph.addEdge(&projects.at(l[1]), &project);
				}
			}

			graph.visitAllNodes();
		} else if (argc == 4 && std::string(argv[1]) == "clone") {
			std::string dir = std::string(argv[3]) + "/";
			git::clone(std::string(argv[2]), "master", dir);
			utils::Cwd cwd {dir};
			std::string call = std::string(argv[0]);
			system(call.c_str());
			call += " test";
			system(call.c_str());

		} else if (argc == 2 && std::string(argv[1]) == "pull") {
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
		} else if (argc == 2 && std::string(argv[1]) == "push") {
			auto allPackages = utils::listDirs("./packages", true);
			for (auto& p : allPackages) { p = "./packages/"+p; }
			allPackages.push_back(".");

			utils::runParallel<std::string>(allPackages, [](std::string const& file) {
				utils::Cwd cwd(file);
				git::push();
			});

		} else if (argc == 2 && std::string(argv[1]) == "test") {
			auto allTests = utils::listFiles("./bin/tests/");
			std::cout<<"===Start testing==="<<std::endl;
			for (auto const& t : allTests) {
				auto p = std::string("./bin/tests/")+t;
				system(p.c_str());
				std::cout<<" • running "<<p<<std::endl;
			}
			std::cout<<"ran "<<allTests.size()<<" test(s)"<<std::endl;
			std::cout<<"===Ended testing==="<<std::endl;
		} else if (argc == 2 && std::string(argv[1]) == "install") {
			auto allFiles = utils::listFiles("./bin/");
			for (auto const& f : allFiles) {
				std::cout<<"installing "<<f<<"; Error Code: ";
				auto oldFile = std::string("./bin/")+f;
				auto newFile = std::string("/usr/bin/")+f;
				auto cpFile  = std::string("cp ")+newFile+" "+oldFile;
				auto error = rename(oldFile.c_str(), newFile.c_str());
				std::cout<<error<<std::endl;
				system(cpFile.c_str());

			}
		}


	} catch(std::exception const& e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}
	return 0;
}
