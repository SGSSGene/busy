#pragma once

#include "BuildAction.h"

namespace aBuild {
	class BuildActionClang : public BuildAction {
	private:
		std::string buildPath;
		std::string libPath;
		std::string objPath;
		std::string execPath;
	public:
		BuildActionClang(Graph const* _graph, bool _verbose, Workspace::ConfigFile const* _configFile)
			: BuildAction(_graph, _verbose, _configFile)
		{
			buildPath = std::string(".aBuild/") + _configFile->getActiveFlavor() + "/";
			libPath   = buildPath + "lib/";
			objPath   = buildPath + "obj/";
			execPath  = "build/" + _configFile->getActiveFlavor() + "/";
		}

		auto getLinkingLibFunc() -> std::function<void(Project*)> override {
			return [this](Project* project) {
				utils::mkdir(libPath);
				std::string call = "ar rcs "+libPath + project->getName()+".a";
				// Get file dependencies
				{
					auto ingoing = graph->getIngoing<std::string, Project>(project, false);
					for (auto const& f : ingoing) {
						call += " " + objPath + *f + ".o";
					}
				}
//				std::cout<<call<<std::endl;
				utils::runProcess(call);
			};
		}

		auto getLinkingExecFunc() -> std::function<void(Project*)> override {
			return [this](Project* project) {
				std::string binPath  = execPath;
				if (utils::isStartingWith(project->getPackagePath(), "packages/")) {
					auto l = utils::explode(project->getPackagePath(), "/");
					binPath = execPath + l[l.size()-1] + "/";
				}
				std::string testPath = binPath + "tests/";
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
					auto ingoing = graph->getIngoing<std::string, Project>(project, false);
					for (auto const& f : ingoing) {
						call += " " + objPath + *f + ".o";
					}
				}
				// Get project dependencies
				{
					auto outgoing = graph->getIngoing<Project, Project>(project, true);
					for (auto const& f : outgoing) {
						call += " " + libPath + f->getName()+".a";

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
		auto getCompileCppFileFunc() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				auto l = utils::explode(*f, "/");

				utils::mkdir(objPath + utils::dirname(*f));
				std::string call = "ccache clang++ -Qunused-arguments -ggdb -O0 --std=c++11 "
				                   "-c " + *f + " "
				                   "-o " + objPath + *f + ".o";

				// Get include dependencies
				{
					Project* project = *graph->getOutgoing<Project, std::string>(f, false).begin();
					for (auto const& i : project->getLegacy().includes) {
						call += " -I "+project->getPackagePath()+"/"+i;
					}
					call += " -I "+project->getPackagePath()+"/src/"+project->getPath();
					call += " -I "+project->getPackagePath()+"/src/";

					auto ingoing = graph->getIngoing<Project, Project>(project, true);
					// Adding all defines of dependent libraries
					call += " -DABUILD";
					for (auto const& f : ingoing) {
						call += std::string(" -DABUILD_");
						for (auto const& c : f->getName()) {
							call += std::toupper(c);
						}
					}
					// Adding all includes of dependent libraries
					for (auto const& f : ingoing) {
						call += " -isystem "+f->getPackagePath()+"/src";
						for (auto const& i : f->getLegacy().includes) {
							call += " -isystem "+f->getPackagePath()+"/"+i;
						}
					}
				}
				if (verbose) {
					std::cout<<call<<std::endl;
				}
				utils::runProcess(call);
			};
		}
		auto getCompileCFileFunc() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				auto l = utils::explode(*f, "/");

				utils::mkdir(".aBuild/obj/" + utils::dirname(*f));
				std::string call = "ccache clang -Qunused-arguments -ggdb -O0 --std=c11 "
				                   "-c " + *f + " "
				                   "-o " + objPath + *f + ".o";

				// Get include dependencies
				{
					Project* project = *graph->getOutgoing<Project, std::string>(f, false).begin();
					for (auto const& i : project->getLegacy().includes) {
						call += " -I "+project->getPackagePath()+"/"+i;
					}
					call += " -I "+project->getPackagePath()+"/src/"+project->getPath();
					call += " -I "+project->getPackagePath()+"/src/";

					auto ingoing = graph->getIngoing<Project, Project>(project, true);
					// Adding all defines of dependent libraries
					call += " -DABUILD";
					for (auto const& f : ingoing) {
						call += std::string(" -DABUILD_");
						for (auto const& c : f->getName()) {
							call += std::toupper(c);
						}
					}
					// Adding all includes of dependent libraries
					for (auto const& f : ingoing) {
						call += " -isystem "+f->getPackagePath()+"/src";
						for (auto const& i : f->getLegacy().includes) {
							call += " -isystem "+f->getPackagePath()+"/"+i;
						}
					}
				}
				if (verbose) {
					std::cout<<call<<std::endl;
				}
				utils::runProcess(call);
			};
		}

	};
}
