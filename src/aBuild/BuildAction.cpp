#include "BuildAction.h"

namespace aBuild {

int64_t BuildAction::getFileModTime(std::string const& s) const {
	auto iter = fileModTime.find(s);
	if (iter != fileModTime.end()) {
		return iter->second;
	}
	auto mod = utils::getFileModificationTime(s);
	fileModTime[s] = mod;
	return mod;
}

bool BuildAction::hasFileChanged(std::string const& s) const {
	auto iter = fileChanged.find(s);
	if (iter == fileChanged.end()) {
		return true;
	}
	return iter->second;
}

BuildAction::BuildAction(Graph const* _graph, bool _verbose, Workspace::ConfigFile const* _configFile, Toolchain const& _toolchain)
	: graph      {_graph}
	, verbose    {_verbose}
	, configFile {_configFile}
	, toolchain  {_toolchain}
{
	buildPath = ".aBuild/" + _configFile->getToolchain() + "/" + _configFile->getFlavor() + "/";
	libPath   = buildPath + "lib/";
	objPath   = buildPath + "obj/";
	execPath  = "build/" + _configFile->getToolchain() + "/" + _configFile->getFlavor() + "/";
}


bool BuildAction::runProcess(std::vector<std::string> const& prog, bool _noWarnings, std::unique_lock<std::mutex>& _lock) const {
	bool success = true;
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

	if (p->getStatus() != 0) {
		success = false;
	}
	//std::cerr << TERM_PURPLE << p.cerr() << TERM_RESET;
	//std::cout << TERM_GREEN  << p.cout() << TERM_RESET;
	if (not _noWarnings || p->getStatus() != 0 || verbose) {
		if (p->cout() != "" || p->cerr() != "") {
			std::cout<<std::endl;
			for (auto const& s : prog) {
				std::cout<<" "<<s;
			}
			std::cout<<std::endl;
			std::cout << p->cout() << TERM_RESET;
			std::cerr << p->cerr() << TERM_RESET;
		}
	}
	if (verbose) {
		std::cout<<"==="<<std::endl;
	}
	return success;
}

auto BuildAction::getLinkingLibFunc() -> std::function<bool(Project*)> {
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
			return runProcess(prog, project->getNoWarnings(), lock);
		} else {
			fileChanged[outputFile] = false;
		}
		return true;
	};
}


auto BuildAction::getLinkingExecFunc() -> std::function<bool(Project*)> {
	return [this](Project* project) {
		std::unique_lock<std::mutex> lock(mutex);
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

		bool _fileChanged = false;
		if (not utils::fileExists(outFile)) {
			_fileChanged = true;
		}


		if (configFile->getFlavor() == "release") {
			prog.push_back("-s");
		}

		// Get file dependencies
		{
			auto ingoing = graph->getIngoing<std::string, Project>(project, false);
			for (auto const& f : ingoing) {
				prog.push_back(objPath + *f + ".o");
				if (hasFileChanged(*f)) {
					_fileChanged = true;
				}
			}
		}
		std::vector<std::string> depLibraries;
		// Set all depLibraries libraries
		for (auto const& l : project->getDepLibraries()) {
			depLibraries.push_back(l);
		}


		std::vector<std::string> dependencies;

		std::set<std::string>    dependenciesWholeArchive; //! using -Wl,--whole-archive flag to make sure all static c'tor are being used

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
					if (f->getWholeArchive()) {
						dependenciesWholeArchive.insert(lib);
					}


					// Set all depLibraries libraries
					for (auto const& l : f->getDepLibraries()) {
						std::string lib = l;
						auto iter = std::find(depLibraries.begin(), depLibraries.end(), lib);
						if (iter == depLibraries.end()) {
							depLibraries.push_back(lib);
						}
					}

				}
			}
		}
		// Adding all depndencies to the prog call
		for (auto const& s : dependencies) {
			bool wholeArchive = dependenciesWholeArchive.find(s) != dependenciesWholeArchive.end();
			if (wholeArchive) {
				prog.push_back("-Wl,--whole-archive");
			}
			prog.push_back(s);
			if (wholeArchive) {
				prog.push_back("-Wl,--no-whole-archive");
			}
			if (hasFileChanged(s)) {
				_fileChanged = true;
			}
		}

		//!TODO missing, if system libs changes, this will not be detected
		// Adding all system dependencies at the end
		for (auto const& s : depLibraries) {
			prog.push_back("-l" + s);
		}
		if (_fileChanged) {
			fileChanged[outFile] = true;
			return runProcess(prog, project->getNoWarnings(), lock);
		} else {
			fileChanged[outFile] = false;
		}
		return true;
	};
}


auto BuildAction::getCompileCppFileFunc() -> std::function<bool(std::string*)> {
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
			return true;
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
		auto list = graph->getOutgoing<Project, std::string>(f, false);
		auto iter = list.begin();
		Project* project = *iter;
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
					if ((c >= 'A' and c <= 'Z')
					    or (c >= 'a' and c <= 'z')
					    or (c >= '0' and c <= '9')) {
						def += std::toupper(c);
					} else {
						def += "_";
					}
				}
				prog.push_back(def);
			}

			{
				std::string def = std::string("-DABUILD_");
				for (auto const& c : project->getName()) {
					if ((c >= 'A' and c <= 'Z')
					    or (c >= 'a' and c <= 'z')
					    or (c >= '0' and c <= '9')) {
						def += std::toupper(c);
					} else {
						def += "_";
					}
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
		return runProcess(prog, project->getNoWarnings(), lock);
	};
}
auto BuildAction::getCompileCppFileFuncDep() -> std::function<void(std::string*)> {
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

auto BuildAction::getCompileCFileFunc() -> std::function<bool(std::string*)> {
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
		return runProcess(prog, project->getNoWarnings(), lock);
	};
}

auto BuildAction::getCompileClangCompleteFunc() -> std::function<void(std::string*)> {
	return [this](std::string* f) {
		std::unique_lock<std::mutex> lock(mutex);
		auto l = utils::explode(*f, "/");

		auto prog = toolchain.getCppCompiler();

		prog.push_back("-std=c++11");

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
		std::fstream fs(".clang_complete", std::ios_base::out | std::ios_base::app);

		for (auto const& p : prog) {
			fs<<p<<" ";
		}
		fs<<std::endl;
		//std::ofstream fstream
		//runProcess(prog, project->getNoWarnings(), lock);
	};
}



}

