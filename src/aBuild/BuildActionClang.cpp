#include "BuildActionClang.h"

namespace aBuild {

auto BuildActionClang:: getLinkingExecFunc() -> std::function<bool(Project*)> {
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


}

