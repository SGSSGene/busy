#pragma once

#include "graph.h"

namespace aBuild {
	class BuildAction {
		public:
		static auto getLinkingLibFunc(Graph const& graph) -> std::function<void(Project*)> {
			return [&graph](Project* project) {
				utils::mkdir(".aBuild/lib/");
				std::string call = "ar rcs .aBuild/lib/"+project->getName()+".a";
				// Get file dependencies
				{
					auto ingoing = graph.getIngoing<std::string, Project>(project, false);
					for (auto const& f : ingoing) {
						call += " .aBuild/obj/"+ *f + ".o";
					}
				}
//				std::cout<<call<<std::endl;
				utils::runProcess(call);
			};
		}

		static auto getLinkingExecFunc(Graph const& graph) -> std::function<void(Project*)> {
			return [&graph](Project* project) {
				std::string binPath  = std::string("build/");
				if (utils::isStartingWith(project->getPackagePath(), "packages/")) {
					auto l = utils::explode(project->getPackagePath(), "/");
					binPath = std::string("build/") + l[l.size()-1] + "/";
				}
				std::string testPath = binPath + "test/";
				utils::mkdir(binPath);
				utils::mkdir(testPath);

				std::string outFile = binPath+project->getName();
				if (utils::isStartingWith(project->getName(), "test")) {
					outFile = testPath+project->getName();
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
		}
		static auto getCompileFileFunc(Graph const& graph) -> std::function<void(std::string*)> {
			return [&graph](std::string* f) {
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
//				std::cout<<call<<std::endl;
				utils::runProcess(call);
			};
		}
	};
}
