#pragma once

#include "BuildAction.h"
#include "Process.h"
#include <queue>

#define TERM_RESET                      "\033[0m"
#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_PURPLE                     "\033[35m"


namespace aBuild {
	class BuildActionClang : public BuildAction {
	private:
		std::string buildPath;
		std::string libPath;
		std::string objPath;
		std::string execPath;
	public:
		void runProcess(std::vector<std::string> const& prog) const {
			if (verbose) {
				for (auto const& s : prog) {
					std::cout<<" "<<s;
				}
				std::cout<<std::endl;
			}
			utils::Process p(prog);
			//std::cerr << TERM_PURPLE << p.cerr() << TERM_RESET;
			//std::cout << TERM_GREEN  << p.cout() << TERM_RESET;
			std::cout << p.cout() << TERM_RESET;
			std::cerr << p.cerr() << TERM_RESET;

		}
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
				runProcess(prog);
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
				std::vector<std::string> depLibraries;
				// Set all depLibraries libraries
				for (auto const& l : project->getDepLibraries()) {
					depLibraries.push_back("-l" + l);
				}


				std::vector<std::string> dependencies;

				// Get project dependencies
				{
					std::queue<Project*> projectQueue;
					projectQueue.push(project);

					while (not projectQueue.empty()) {
						auto curProject = projectQueue.front();
						projectQueue.pop();
						auto outgoing = graph->getIngoing<Project, Project>(curProject, true);
						for (auto const& f : outgoing) {
							projectQueue.push(f);
							std::string lib = libPath + f->getName() + ".a";
							auto iter = std::find(dependencies.begin(), dependencies.end(), lib);
							if (iter != dependencies.end()) {
								dependencies.erase(iter);
							}
							dependencies.push_back(lib);

							// Set all depLibraries libraries
							for (auto const& l : f->getDepLibraries()) {
								std::string lib = "-l" + l;
								auto iter = std::find(depLibraries.begin(), depLibraries.end(), lib);
								if (iter != depLibraries.end()) {
									depLibraries.erase(iter);
								}
								depLibraries.push_back(lib);
							}

						}
					}
				}
				// Adding all depndencies to the prog call
				for (auto const& s : dependencies) {
					prog.push_back(s);
				}
				// Adding all system dependencies at the end
				for (auto const& s : depLibraries) {
					prog.push_back(s);
				}
				runProcess(prog);
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
				runProcess(prog);
			};
		}
		auto getCompileCFileFunc() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				auto l = utils::explode(*f, "/");

				utils::mkdir(objPath + utils::dirname(*f));

				auto prog = toolchain.getCCompiler();

				prog.push_back("-std=c11");
				prog.push_back("-Wall");
				prog.push_back("-Wextra");
				prog.push_back("-fmessage-length=0");

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
						std::string def = "-DABUILD_";
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
							prog.push_back(f->getPackagePath()+"/" + i);
						}
					}
				}
				runProcess(prog);
			};
		}

	};
}
