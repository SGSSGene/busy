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
				std::string prog = toolchain.getArchivist();
				std::vector<std::string> parameters;
				parameters.push_back("rcs");
				parameters.push_back(libPath + project->getName() + ".a");
				auto ingoing = graph->getIngoing<std::string, Project>(project, false);
				for (auto const& f : ingoing) {
					parameters.push_back(objPath + *f + ".o");
				}

				if (verbose) {
					std::cout<<prog;
					for (auto const& s : parameters) {
						std::cout<<" "<<s;
					}
					std::cout<<std::endl;
				}
				utils::Process p(prog, parameters);
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
				std::string prog = toolchain.getCppCompiler();
				std::vector<std::string> parameters;
				parameters.push_back("-o");
				parameters.push_back(outFile);

				if (configFile->getFlavor() == "release") {
					parameters.push_back("-s");
				}

				// Get file dependencies
				{
					auto ingoing = graph->getIngoing<std::string, Project>(project, false);
					for (auto const& f : ingoing) {
						parameters.push_back(objPath + *f + ".o");
					}
				}
				// Set all depLibraries libraries
				for (auto const& l : project->getDepLibraries()) {
					parameters.push_back("-l"+l);
				}

				// Get project dependencies
				{
					auto outgoing = graph->getIngoing<Project, Project>(project, true);
					for (auto const& f : outgoing) {
						parameters.push_back(libPath + f ->getName() + ".a");

						// Set all depLibraries libraries
						for (auto const& l : f->getDepLibraries()) {
							parameters.push_back("-l"+l);
						}

					}
				}
				if (verbose) {
					std::cout<<prog;
					for (auto const& s : parameters) {
						std::cout<<" "<<s;
					}
					std::cout<<std::endl;
				}

				utils::Process p(prog, parameters);
				std::cerr<<p.cerr();
				std::cout<<p.cout();

			};
		}
		auto getCompileCppFileFunc() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				auto l = utils::explode(*f, "/");

				std::string prog = toolchain.getCppCompiler();
				std::vector<std::string> parameters;

				parameters.push_back("-std=c++11");
				parameters.push_back("-Wall");
				parameters.push_back("-Wextra");
				parameters.push_back("-fmessage-length=0");


				utils::mkdir(objPath + utils::dirname(*f));
				if (configFile->getFlavor() == "release") {
					parameters.push_back("-O2");
				} else if (configFile->getFlavor() == "debug") {
					parameters.push_back("-ggdb");
					parameters.push_back("-O0");
				}
				parameters.push_back("-c");
				parameters.push_back(*f);
				parameters.push_back("-o");
				parameters.push_back(objPath + *f + ".o");

				// Get include dependencies
				{
					Project* project = *graph->getOutgoing<Project, std::string>(f, false).begin();
					for (auto const& i : project->getLegacy().includes) {
						parameters.push_back("-I");
						parameters.push_back(project->getPackagePath()+"/"+i);
					}
					parameters.push_back("-I");
					parameters.push_back(project->getPackagePath()+"/src/"+project->getPath());
					parameters.push_back("-I");
					parameters.push_back(project->getPackagePath()+"/src/");

					auto ingoing = graph->getIngoing<Project, Project>(project, true);
					// Adding all defines of dependent libraries
					parameters.push_back("-DABUILD");

					for (auto const& f : ingoing) {
						std::string def = std::string("-DABUILD_");
						for (auto const& c : f->getName()) {
							def += std::toupper(c);
						}
						parameters.push_back(def);

					}
					// Adding all includes of dependent libraries
					for (auto const& f : ingoing) {
						parameters.push_back("-isystem");
						parameters.push_back(f->getPackagePath()+"/src");
						for (auto const& i : f->getLegacy().includes) {
							parameters.push_back("-isystem");
							parameters.push_back(f->getPackagePath()+"/"+i);
						}
					}
				}
				if (verbose) {
					std::cout<<prog;
					for (auto const& s : parameters) {
						std::cout<<" "<<s;
					}
					std::cout<<std::endl;
				}
				utils::Process p(prog, parameters);
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
				std::string call = toolchain.getCCompiler() + flags + " "
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
