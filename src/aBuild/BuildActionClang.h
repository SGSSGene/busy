#pragma once

#include <iostream>
#include <fstream>
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
		mutable std::map<std::string, int64_t> fileModTime; // caching
		mutable std::map<std::string, bool>    fileChanged; // caching

		std::mutex mutex;

		int64_t getFileModTime(std::string const& s) const {
			auto iter = fileModTime.find(s);
			if (iter != fileModTime.end()) {
				return iter->second;
			}
			auto mod = utils::getFileModificationTime(s);
			fileModTime[s] = mod;
			return mod;
		}
		bool hasFileChanged(std::string const& s) const {
			auto iter = fileChanged.find(s);
			if (iter == fileChanged.end()) {
				return true;
			}
			return iter->second;
		}
	public:
		void runProcess(std::vector<std::string> const& prog, bool _noWarnings, std::unique_lock<std::mutex>& _lock) const {
			if (verbose) {
				for (auto const& s : prog) {
					std::cout<<" "<<s;
				}
				std::cout<<std::endl;
			}

			std::condition_variable cv;
			std::unique_ptr<utils::Process> p;
			std::thread t ([&]{
				p.reset(new utils::Process(prog));
				std::unique_lock<std::mutex> lock(*_lock.mutex());
				cv.notify_one();
			});
			cv.wait(_lock);
			t.join();
			//std::cerr << TERM_PURPLE << p.cerr() << TERM_RESET;
			//std::cout << TERM_GREEN  << p.cout() << TERM_RESET;
			if (not _noWarnings || p->getStatus() != 0 || verbose) {
				std::cout << p->cout() << TERM_RESET;
				std::cerr << p->cerr() << TERM_RESET;
			}
			if (verbose) {
				std::cout<<"==="<<std::endl;
			}


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
				std::unique_lock<std::mutex> lock(mutex);
				utils::mkdir(libPath);
				auto prog = toolchain.getArchivist();
				prog.push_back("rcs");
				std::string outputFile = libPath + project->getName() + ".a";
				prog.push_back(outputFile);
				bool _fileChanged = false;
				if (not utils::fileExists(outputFile)) {
					_fileChanged = true;
				}
				auto ingoing = graph->getIngoing<std::string, Project>(project, false);
				for (auto const& f : ingoing) {
					prog.push_back(objPath + *f + ".o");
					if (hasFileChanged(*f)) {
						_fileChanged = true;
					}
				}
				if (_fileChanged) {
					fileChanged[outputFile] = true;
					runProcess(prog, project->getNoWarnings(), lock);
				} else {
					fileChanged[outputFile] = false;
				}
			};
		}

		auto getLinkingExecFunc() -> std::function<void(Project*)> override;

		auto getCompileCppFileFunc() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				std::unique_lock<std::mutex> lock(mutex);
				auto l = utils::explode(*f, "/");

				auto prog = toolchain.getCppCompiler();

				prog.push_back("-std=c++11");
				prog.push_back("-Wall");
				prog.push_back("-Wextra");
				prog.push_back("-fmessage-length=0");

				std::string inputFile  = *f;
				std::string outputFile = objPath + *f + ".o";

				auto outputFileMod = getFileModTime(outputFile);

				auto nothingChanged = true;
				if (not utils::fileExists(outputFile)) {
					nothingChanged = false;
				} else if (outputFileMod < getFileModTime(inputFile)) {
					nothingChanged = false;
				} else if (not utils::fileExists(objPath + *f + ".d")) {
					nothingChanged = false;
				} else {
					std::ifstream ifs(objPath + *f + ".d");
					for (std::string line; std::getline(ifs, line);) {
						if (not utils::fileExists(line)) {
							nothingChanged = false;
							break;
						} else if (outputFileMod < getFileModTime(line)) {
							nothingChanged = false;
							break;
						}
					}
				}


				if (nothingChanged) {
					fileChanged[*f] = false;
					return;
				}

				prog.push_back("-c");
				prog.push_back(inputFile);
				prog.push_back("-o");
				prog.push_back(outputFile);

				utils::mkdir(objPath + utils::dirname(*f));
				if (configFile->getFlavor() == "release") {
					prog.push_back("-O2");
				} else if (configFile->getFlavor() == "debug") {
					prog.push_back("-ggdb");
					prog.push_back("-O0");
				}

				// Get include dependencies
				Project* project = *graph->getOutgoing<Project, std::string>(f, false).begin();
				{
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
				runProcess(prog, project->getNoWarnings(), lock);
			};
		}
		auto getCompileCppFileFuncDep() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				std::unique_lock<std::mutex> lock(mutex);
				if (not hasFileChanged(*f)) {
					return;
				}

				auto l = utils::explode(*f, "/");

				auto prog = toolchain.getCppCompiler();


				prog.push_back("-std=c++11");
				prog.push_back("-Wall");
				prog.push_back("-Wextra");
				prog.push_back("-fmessage-length=0");

				prog.push_back("-c");
				prog.push_back(*f);
				prog.push_back("-M");
				prog.push_back("-MF");
				prog.push_back("/dev/stdout");

				utils::mkdir(objPath + utils::dirname(*f));
				if (configFile->getFlavor() == "release") {
					prog.push_back("-O2");
				} else if (configFile->getFlavor() == "debug") {
					prog.push_back("-ggdb");
					prog.push_back("-O0");
				}

				// Get include dependencies
				Project* project = *graph->getOutgoing<Project, std::string>(f, false).begin();
				{
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
				utils::Process p(prog);
				if (p.getStatus() == 0) {
					auto depFiles = utils::explode(p.cout(), std::vector<std::string> {"\n", " ", "\\"});

					std::ofstream ofs(objPath + *f + ".d");
					for (auto iter = ++depFiles.begin(); iter != depFiles.end(); ++iter) {
						if (iter->length() > 0) {
							ofs << *iter << std::endl;
						}
					}
				}
			};
		}

		auto getCompileCFileFunc() -> std::function<void(std::string*)> override {
			return [this](std::string* f) {
				std::unique_lock<std::mutex> lock(mutex);
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
				Project* project = *graph->getOutgoing<Project, std::string>(f, false).begin();
				{
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
				runProcess(prog, project->getNoWarnings(), lock);
			};
		}

	};
}
