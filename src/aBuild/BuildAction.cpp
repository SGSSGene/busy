#include "BuildAction.h"

#include <process/Process.h>
#include "FileStates.h"

namespace aBuild {

BuildAction::BuildAction(Graph const* _graph, bool _verbose, Workspace::ConfigFile const* _configFile, Toolchain const& _toolchain)
	: mGraph      {_graph}
	, mVerbose    {_verbose}
	, mConfigFile {_configFile}
	, mToolchain  {_toolchain}
{
	mBuildPath = ".aBuild/" + mConfigFile->getToolchain() + "/" + mConfigFile->getBuildMode() + "/";
	mLibPath   = mBuildPath + "lib/";
	mObjPath   = mBuildPath + "obj/";
	mExecPath  = "build/" + mConfigFile->getToolchain() + "/" + mConfigFile->getBuildMode() + "/";
}


bool BuildAction::runProcess(std::vector<std::string> const& prog, bool _noWarnings) const {
	bool success = true;
	if (mVerbose) {
		for (auto const& s : prog) {
			std::cout<<" "<<s;
		}
		std::cout<<std::endl;
	}

	process::Process p(prog);

	if (p.getStatus() != 0) {
		success = false;
	}
	//std::cerr << TERM_PURPLE << p.cerr() << TERM_RESET;
	//std::cout << TERM_GREEN  << p.cout() << TERM_RESET;
	if (not _noWarnings or p.getStatus() != 0 or mVerbose) {
		if (p.cout() != "" || p.cerr() != "") {
			std::cout<<std::endl;
			for (auto const& s : prog) {
				std::cout<<" "<<s;
			}
			std::cout<<std::endl;
			std::cout << p.cout() << TERM_RESET;
			std::cerr << p.cerr() << TERM_RESET;
		}
	}
	if (mVerbose) {
		std::cout<<"==="<<std::endl;
	}
	return success;
}

auto BuildAction::getLinkingLibFunc() -> std::function<bool(Project*)> {
	return [this](Project* project) {
		std::unique_lock<std::mutex> lock(mMutex);
		utils::mkdir(mLibPath);
		auto prog = mToolchain.getArchivist();
		prog.push_back("rcs");
		std::string outputFile = mLibPath + project->getName() + ".a";
		prog.push_back(outputFile);

		bool _fileChanged = false;
		// is output file available
		if (not utils::fileExists(outputFile)) {
			_fileChanged = true;
		}

		// did any of the object files changed
		auto ingoing = mGraph->getIngoing<std::string, Project>(project, false);
		for (auto const& f : ingoing) {
			prog.push_back(mObjPath + *f + ".o");
			if (getFileStates().hasFileChanged(*f)) {
				_fileChanged = true;
			}
		}
		// if something changed compile
		getFileStates().setFileChanged(outputFile, _fileChanged);
		if (_fileChanged) {
			lock.unlock();
			return runProcess(prog, project->getNoWarnings());
		}
		return true;
	};
}


auto BuildAction::getLinkingExecFunc() -> std::function<bool(Project*)> {
	return [this](Project* project) {
		std::unique_lock<std::mutex> lock(mMutex);
		std::string binPath  = mExecPath;
		if (utils::isStartingWith(project->getPackagePath(), "packages/")) {
			auto l = utils::explode(project->getPackagePath(), "/");
			binPath = mExecPath + l[l.size()-1] + "/";
		}
		std::string testPath = binPath + "tests/";
		utils::mkdir(binPath);
		utils::mkdir(testPath);

		std::string outputFile = binPath+project->getName();
		if (utils::isStartingWith(project->getName(), "test")) {
			outputFile = testPath+project->getName();
		}
		auto prog = mToolchain.getCppCompiler();
		prog.push_back("-o");
		prog.push_back(outputFile);

		bool _fileChanged = false;
		if (not utils::fileExists(outputFile)) {
			_fileChanged = true;
		}


		if (mConfigFile->getBuildMode() == "release") {
			prog.push_back("-s");
		}

		// Get file dependencies
		{
			auto ingoing = mGraph->getIngoing<std::string, Project>(project, false);
			for (auto const& f : ingoing) {
				prog.push_back(mObjPath + *f + ".o");
				if (getFileStates().hasFileChanged(*f)) {
					_fileChanged = true;
				}
			}
		}
		std::vector<std::string> depLibraries;
		// Set all depLibraries libraries
		for (auto const& l : project->getDepLibraries()) {
			depLibraries.push_back(l);
		}


		// Get systemLibraries paths
		{
			auto ingoing = mGraph->getIngoing<Project, Project>(project, true);
			for (auto const& f : ingoing) {
				for (auto const& i : f->getLegacy().systemLibraries) {
					prog.push_back("-L");
					prog.push_back(i);
				}
			}
			for (auto const& f : ingoing) {
				for (auto const& i : f->getLegacy().linkingOption) {
					prog.push_back(i);
				}
			}
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
				auto outgoing = mGraph->getIngoing<Project, Project>(curProject, true);
				for (auto const& f : outgoing) {
					projectQueue.push(f);
					std::string lib = mLibPath + f->getName() + ".a";
					auto iter = std::find(dependencies.begin(), dependencies.end(), lib);
					if (iter != dependencies.end()) {
						dependencies.erase(iter);
					}
					dependencies.push_back(lib);

					// check if any this project or one of its dependencies is a wholeArchive
					bool wholeArchive = f->getWholeArchive();
					auto outgoingSubProject = mGraph->getIngoing<Project, Project>(f, true);
					for (auto const& p : outgoingSubProject) {
						if (p->getWholeArchive()) {
							wholeArchive = true;
							break;
						}
					}
					if (wholeArchive) {
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
			if (getFileStates().hasFileChanged(s)) {
				_fileChanged = true;
			}
		}

		//!TODO missing, if system libs changes, this will not be detected
		// Adding all system dependencies at the end
		for (auto const& s : depLibraries) {
			prog.push_back("-l" + s);
		}

		getFileStates().setFileChanged(outputFile, _fileChanged);
		if (_fileChanged) {
			lock.unlock();
			return runProcess(prog, project->getNoWarnings());
		}
		return true;
	};
}


auto BuildAction::getCompileCppFileFunc() -> std::function<bool(std::string*)> {
	return [this](std::string* f) {
		std::unique_lock<std::mutex> lock(mMutex);

		std::string inputFile  = *f;
		std::string outputFile = mObjPath + *f + ".o";

		auto changed = fileOrDependencyChanged(inputFile, outputFile);
		// if nothing changed, no need to compile
		getFileStates().setFileChanged(outputFile, changed);
		getFileStates().setFileChanged(inputFile, changed);
		if (not changed) {
			return true;
		}

		auto l = utils::explode(*f, "/");

		auto prog = mToolchain.getCppCompiler();

		prog.push_back("-std=c++11");
		prog.push_back("-Wall");
		prog.push_back("-Wextra");
		prog.push_back("-fmessage-length=0");
		prog.push_back("-fmax-errors=3");
		prog.push_back("-c");
		prog.push_back(inputFile);
		prog.push_back("-o");
		prog.push_back(outputFile);

		utils::mkdir(mObjPath + utils::dirname(*f));
		if (mConfigFile->getBuildMode() == "release") {
			prog.push_back("-O3");
		} else if (mConfigFile->getBuildMode() == "debug") {
			prog.push_back("-ggdb");
			prog.push_back("-O0");
		}

		// Get include dependencies
		auto list = mGraph->getOutgoing<Project, std::string>(f, false);
		auto iter = list.begin();
		Project* project = *iter;

		for (auto const& e : getIncludeAndDefines(project)) {
			prog.push_back(e);
		}

		lock.unlock();
		return runProcess(prog, project->getNoWarnings());
	};
}
auto BuildAction::getCompileCppFileFuncDep() -> std::function<void(std::string*)> {
	return [this](std::string* f) {
		std::unique_lock<std::mutex> lock(mMutex);

		std::string inputFile  = *f;
		std::string outputFile = mObjPath + *f + ".o";

		auto changed = fileOrDependencyChanged(inputFile, outputFile);
		// if nothing changed, no need to compile
		getFileStates().setFileChanged(outputFile, changed);
		getFileStates().setFileChanged(inputFile, changed);
		if (not changed) {
			return;
		}

		auto l = utils::explode(*f, "/");

		auto prog = mToolchain.getCppCompiler();


		prog.push_back("-std=c++11");

		prog.push_back("-c");
		prog.push_back(*f);
		prog.push_back("-M");
		prog.push_back("-MF");
		prog.push_back("/dev/stdout");

		utils::mkdir(mObjPath + utils::dirname(*f));

		// Get include dependencies
		Project* project = *mGraph->getOutgoing<Project, std::string>(f, false).begin();

		for (auto const& e : getIncludeAndDefines(project)) {
			prog.push_back(e);
		}

		if (mVerbose) {
			for (auto const& s : prog) {
				std::cout<<" "<<s;
			}
			std::cout<<std::endl;
		}

		lock.unlock();
		process::Process p(prog);
		if (p.getStatus() == 0) {
			auto depFiles = utils::explode(p.cout(), std::vector<std::string> {"\n", " ", "\\"});

			std::ofstream ofs(mObjPath + *f + ".d");
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
		std::unique_lock<std::mutex> lock(mMutex);

		std::string inputFile  = *f;
		std::string outputFile = mObjPath + *f + ".o";

		auto changed = fileOrDependencyChanged(inputFile, outputFile);

		// if nothing changed, no need to compile
		getFileStates().setFileChanged(outputFile, changed);
		getFileStates().setFileChanged(inputFile, changed);
		if (not changed) {
			return true;
		}

		auto l = utils::explode(*f, "/");

		utils::mkdir(mObjPath + utils::dirname(*f));

		auto prog = mToolchain.getCCompiler();

		prog.push_back("-std=c11");
		prog.push_back("-Wall");
		prog.push_back("-Wextra");
		prog.push_back("-fmessage-length=0");
		prog.push_back("-fmax-errors=3");
		prog.push_back("-c");
		prog.push_back(*f);
		prog.push_back("-o");
		prog.push_back(mObjPath + *f + ".o");

		utils::mkdir(mObjPath + utils::dirname(*f));
		if (mConfigFile->getBuildMode() == "release") {
			prog.push_back("-O3");
		} else if (mConfigFile->getBuildMode() == "debug") {
			prog.push_back("-ggdb");
			prog.push_back("-O0");
		}


		// Get include dependencies
		Project* project = *mGraph->getOutgoing<Project, std::string>(f, false).begin();

		for (auto const& e : getIncludeAndDefines(project)) {
			prog.push_back(e);
		}

		lock.unlock();
		return runProcess(prog, project->getNoWarnings());
	};
}

auto BuildAction::getCompileCFileFuncDep() -> std::function<void(std::string*)> {
	return [this](std::string* f) {
		std::unique_lock<std::mutex> lock(mMutex);

		std::string inputFile  = *f;
		std::string outputFile = mObjPath + *f + ".o";

		auto changed = fileOrDependencyChanged(inputFile, outputFile);
		// if nothing changed, no need to compile
		getFileStates().setFileChanged(outputFile, changed);
		getFileStates().setFileChanged(inputFile, changed);
		if (not changed) {
			return;
		}

		auto l = utils::explode(*f, "/");

		auto prog = mToolchain.getCppCompiler();


		prog.push_back("-std=c11");

		prog.push_back("-c");
		prog.push_back(*f);
		prog.push_back("-M");
		prog.push_back("-MF");
		prog.push_back("/dev/stdout");

		utils::mkdir(mObjPath + utils::dirname(*f));

		// Get include dependencies
		Project* project = *mGraph->getOutgoing<Project, std::string>(f, false).begin();

		for (auto const& e : getIncludeAndDefines(project)) {
			prog.push_back(e);
		}

		if (mVerbose) {
			for (auto const& s : prog) {
				std::cout<<" "<<s;
			}
			std::cout<<std::endl;
		}

		lock.unlock();
		process::Process p(prog);
		if (p.getStatus() == 0) {
			auto depFiles = utils::explode(p.cout(), std::vector<std::string> {"\n", " ", "\\"});

			std::ofstream ofs(mObjPath + *f + ".d");
			for (auto iter = ++depFiles.begin(); iter != depFiles.end(); ++iter) {
				if (iter->length() > 0) {
					ofs << *iter << std::endl;
				}
			}
		}
	};
}


auto BuildAction::getCompileClangCompleteFunc() -> std::function<void(std::string*)> {
	return [this](std::string* f) {
		std::unique_lock<std::mutex> lock(mMutex);
		auto l = utils::explode(*f, "/");

		auto prog = mToolchain.getCppCompiler();
		prog.erase(prog.begin());

		prog.push_back("-std=c++11");

		// Get include dependencies
		Project* project = *mGraph->getOutgoing<Project, std::string>(f, false).begin();

		for (auto const& e : getIncludeAndDefines(project)) {
			prog.push_back(e);
		}

		std::fstream fs(".clang_complete", std::ios_base::out | std::ios_base::app);
		for (auto const& p : prog) {
			fs<<p<<" ";
		}
		fs<<std::endl;
	};
}

auto BuildAction::getIncludeAndDefines(Project* project) const -> std::vector<std::string> {
	std::vector<std::string> retList;

	auto ingoing = mGraph->getIngoing<Project, Project>(project, true);

	// Adding all defines of dependend libraries
	for (auto const& d : project->getComDefines(ingoing)) {
		retList.push_back(d);
	}

	// aAding all includes depending inside of the project
	for (auto const& i : project->getComIncludePaths()) {
		retList.push_back("-I");
		retList.push_back(i);
	}

	// Adding all includes of dependening libraries
	for (auto const& i : project->getComSystemIncludePaths(ingoing)) {
		retList.push_back("-isystem");
		retList.push_back(i);
	}

	return retList;
}

bool BuildAction::fileOrDependencyChanged(std::string const& _inputFile, std::string const& _outputFile) const {
	if (not utils::fileExists(_outputFile)) {
		return true;
	}

	auto outputFileMod = getFileStates().getFileModTime(_outputFile);
	auto inputFileMod  = getFileStates().getFileModTime(_inputFile);
	if (outputFileMod > mConfigFile->getLastCompileTime()) {
		outputFileMod = mConfigFile->getLastCompileTime();
	}

	if (outputFileMod < inputFileMod) {
		return true;
	}

	if (not utils::fileExists(mObjPath + _inputFile + ".d")) {
		return true;
	}

	std::ifstream ifs(mObjPath + _inputFile + ".d");
	for (std::string line; std::getline(ifs, line);) {
		if (not utils::fileExists(line)) {
			return true;
		}
		auto inputFileMod  = getFileStates().getFileModTime(line);

		if (outputFileMod < inputFileMod) {
			return true;
		}
	}

	return false;
}
}
