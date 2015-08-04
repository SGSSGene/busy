#include "Workspace.h"

#include <memory>
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

			git::clone(url.getURL(), url.getBranch(), url.getPath());
		}
		missingPackages = ws.getAllMissingPackages();
	}
}
static void checkingNotNeededPackages(Workspace& ws) {
	// Checking not needed packages
	auto notRequiredPackages = ws.getAllNotRequiredPackages();
	while (not notRequiredPackages.empty()) {
		for (auto const& s : notRequiredPackages) {
			std::cout<<"Not Required "<<s<<std::endl;
			utils::rm(std::string("packages/")+s, true, true);
		}
		notRequiredPackages = ws.getAllNotRequiredPackages();
	}
}
static void checkingInvalidPackages(Workspace& ws) {
	// checking invalid packages
	auto invalidPackages = ws.getAllInvalidPackages();
	for (auto const& p : invalidPackages) {
		std::cout<<"Package is ill formed: "<<p<<std::endl;
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

		} else if (argc == 2 && std::string(argv[1]) == "pull") {
			auto allPackages = utils::listDirs("./packages", true);
			//!TODO This should be done in parallel
			for (auto const& s : allPackages) {
				auto file = std::string("./packages/") + s;
				utils::Cwd cwd(file);
				if (git::isDirty()) {
					std::cout<<"ignore " << file << ": Dirty repository"<<std::endl;
				} else {
					git::pull();
				}
			}
		} else if (argc == 2 && std::string(argv[1]) == "push") {
			auto allPackages = utils::listDirs("./packages", true);
			//!TODO This should be done in parallel
			for (auto const& s : allPackages) {
				auto file = std::string("./packages/") + s;
				utils::Cwd cwd(file);
				git::push();
			}
		} else if (argc == 2 && std::string(argv[1]) == "test") {
			auto allTests = utils::listFiles("./bin/tests/");
			std::cout<<"===Start testing==="<<std::endl;
			std::vector<std::string> failed;
			for (auto const& t : allTests) {
				auto p = std::string("./bin/tests/")+t;
				auto retValue = utils::runProcess(p);
				std::cout<<" • running "<<p<<std::endl;
				if (not retValue.empty()) {
					std::cout<<retValue<<std::endl;
				}
			}
			std::cout<<"===Ended testing==="<<std::endl;
		}

	} catch(std::exception const& e) {
		std::cerr<<"exception: "<<e.what()<<std::endl;
	}
	return 0;
}
