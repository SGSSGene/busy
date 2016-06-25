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
		Graph const*                 mGraph;
		bool const                   mVerbose;
		Workspace::ConfigFile const* mConfigFile;
		Toolchain                    mToolchain;

		std::string mBuildPath;
		std::string mLibPath;
		std::string mObjPath;
		std::string mExecPath;

		std::mutex mMutex;
	public:
		BuildAction(Graph const* _graph, bool _verbose, Workspace::ConfigFile const* _configFile, Toolchain const& _toolchain);
		bool runProcess(std::vector<std::string> const& prog, bool _noWarnings) const;

		auto getLinkingLibFunc()                      -> std::function<bool(Project*)>;
		auto getLinkingExecFunc(bool _shared = false) -> std::function<bool(Project*)>;
		auto getCompileFunc(std::string _std)         -> std::function<bool(std::string*)>;
		auto getCompileClangCompleteFunc()            -> std::function<void(std::string*)>;
	private:

		auto getIncludeAndDefines(Project* project) const -> std::vector<std::string>;

		bool fileOrDependencyChanged(std::string const& _inputFile, std::string const& _outputFile) const;
	};
}
