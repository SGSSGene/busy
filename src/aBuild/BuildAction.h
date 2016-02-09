#pragma once

#include <iostream>
#include <fstream>
#include <queue>
#include "graph.h"
#include "Workspace.h"


#define TERM_RESET                      "\033[0m"
#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_PURPLE                     "\033[35m"


namespace aBuild {
	class BuildAction {
	private:
		Graph const* graph;
		const bool verbose;
		Workspace::ConfigFile const* configFile;
		Toolchain toolchain;

		std::string buildPath;
		std::string libPath;
		std::string objPath;
		std::string execPath;
		mutable std::map<std::string, int64_t> fileModTime; // caching
		mutable std::map<std::string, bool>    fileChanged; // caching

		std::mutex mutex;

		int64_t getFileModTime(std::string const& s) const;
		bool hasFileChanged(std::string const& s) const;

	public:
		BuildAction(Graph const* _graph, bool _verbose, Workspace::ConfigFile const* _configFile, Toolchain const& _toolchain);
		bool runProcess(std::vector<std::string> const& prog, bool _noWarnings) const;


		auto getLinkingLibFunc()           -> std::function<bool(Project*)>;
		auto getLinkingExecFunc()          -> std::function<bool(Project*)>;
		auto getCompileCppFileFunc()       -> std::function<bool(std::string*)>;
		auto getCompileCppFileFuncDep()    -> std::function<void(std::string*)>;
		auto getCompileCFileFunc()         -> std::function<bool(std::string*)>;
		auto getCompileClangCompleteFunc() -> std::function<void(std::string*)>;
	};
}
