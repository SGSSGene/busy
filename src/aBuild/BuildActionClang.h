#pragma once

#include "BuildAction.h"
#include "Process.h"

namespace aBuild {
	class BuildActionClang : public BuildAction {
	private:
		std::string buildPath;
		std::string libPath;
		std::string objPath;
		std::string execPath;

	public:
		BuildActionClang(Graph const* _graph, bool _verbose, Workspace::ConfigFile const* _configFile, Toolchain const& _toolchain)
			: BuildAction(_graph, _verbose, _configFile, _toolchain)
		{
			buildPath = ".aBuild/" + _configFile->getToolchain() + "/" + _configFile->getFlavor() + "/";
			libPath   = buildPath + "lib/";
			objPath   = buildPath + "obj/";
			execPath  = "build/" + _configFile->getToolchain() + "/" + _configFile->getFlavor() + "/";
		}

		auto getLinkingLibFunc() -> std::function<void(Project*)> override {
			return [this](Project* project) {
				utils::mkdir(libPath);
				auto prog = toolchain.getArchivist();
				prog.push_back("rcs");
				prog.push_back(libPath + project->getName() + ".a");
				auto ingoing = graph->getIngoing<std::string, Project>(project, false);
				for (auto const& f : ingoing) {
					prog.push_back(objPath + *f + ".o");
				}

				if (verbose) {
					for (auto const& s : prog) {
						std::cout<<" "<<s;
					}
					std::cout<<std::endl;
				}
				utils::Process p(prog);
				std::cerr<<p.cerr();
				std::cout<<p.cout();
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
				auto prog = toolchain.getCppCompiler();
				prog.push_back("-o");
				prog.push_back(outFile);

				if (configFile->getFlavor() == "release") {
					prog.push_back("-s");
				}

				// Get file dependencies
				{
					auto ingoing = graph->getIngoing<std::string, Project>(project, false);
					for (auto const& f : ingoing) {
						prog.push_back(objPath + *f + ".o");
					}
				}
				// Set all depLibraries libraries
				for (auto const& l : project->getDepLibraries()) {
					prog.push_back("-l"+l);
				}

				// Get project dependencies
				{
					auto outgoing = graph->getIngoing<Project, Project>(project, true);
					for (auto const& f : outgoing) {
						prog.push_back(libPath + f ->getName() + ".a");

						// Set all depLibraries libraries
						for (auto const& l : f->getDepLibraries()) {
							prog.push_back("-l"+l);
						}

					}
				}
				if (verbose) {
					for (auto const& s : prog) {
						std::cout<<" "<<s;
					}
					std::cout<<std::endl;
				}

				utils::Process p(prog);
				std::cerr<<p.cerr();
				std::cout<<p.cout();

			};
		}
		auto getCompileCppFileFunc() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				auto l = utils::explode(*f, "/");

				auto prog = toolchain.getCppCompiler();

				prog.push_back("-std=c++11");
				prog.push_back("-Wall");
				prog.push_back("-Wextra");
				prog.push_back("-fmessage-length=0");


				utils::mkdir(objPath + utils::dirname(*f));
				if (configFile->getFlavor() == "release") {
					prog.push_back("-O2");
				} else if (configFile->getFlavor() == "debug") {
					prog.push_back("-ggdb");
					prog.push_back("-O0");
				}
				prog.push_back("-c");
				prog.push_back(*f);
				prog.push_back("-o");
				prog.push_back(objPath + *f + ".o");

				// Get include dependencies
				{
					Project* project = *graph->getOutgoing<Project, std::string>(f, false).begin();
					for (auto const& i : project->getLegacy().includes) {
						prog.push_back("-I");
						prog.push_back(project->getPackagePath()+"/"+i);
					}
					prog.push_back("-I");
					prog.push_back(project->getPackagePath()+"/src/"+project->getPath());
					prog.push_back("-I");
					prog.push_back(project->getPackagePath()+"/src/");

					auto ingoing = graph->getIngoing<Project, Project>(project, true);
					// Adding all defines of dependent libraries
					prog.push_back("-DABUILD");

					for (auto const& f : ingoing) {
						std::string def = std::string("-DABUILD_");
						for (auto const& c : f->getName()) {
							def += std::toupper(c);
						}
						prog.push_back(def);

					}
					// Adding all includes of dependent libraries
					for (auto const& f : ingoing) {
						prog.push_back("-isystem");
						prog.push_back(f->getPackagePath()+"/src");
						for (auto const& i : f->getLegacy().includes) {
							prog.push_back("-isystem");
							prog.push_back(f->getPackagePath()+"/"+i);
						}
					}
				}
				if (verbose) {
					for (auto const& s : prog) {
						std::cout<<" "<<s;
					}
					std::cout<<std::endl;
				}
				utils::Process p(prog);
				std::cerr<<p.cerr();
				std::cout<<p.cout();
			};
		}
		auto getCompileCFileFunc() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				auto l = utils::explode(*f, "/");

				utils::mkdir(objPath + utils::dirname(*f));
				std::string flags = " -std=c11 -Wall -Wextra -fmessage-length=0";
				if (configFile->getFlavor() == "release") {
					flags += " -O2";
				} else if (configFile->getFlavor() == "debug") {
					flags += " -ggdb -O0";
				}
				std::string call = toolchain.getCCompiler()[0] + flags + " "
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
