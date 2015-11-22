#pragma once

#include "graph.h"
#include "Workspace.h"

namespace aBuild {
	class BuildAction {
	protected:
		Graph const* graph;
		const bool verbose;
		Workspace::ConfigFile const* configFile;
		Toolchain toolchain;
	public:
		BuildAction(Graph const* _graph, bool _verbose, Workspace::ConfigFile const* _configFile, Toolchain const& _toolchain)
			: graph      {_graph}
			, verbose    {_verbose}
			, configFile {_configFile}
			, toolchain  {_toolchain}
			{}
		virtual auto getLinkingLibFunc    ()         -> std::function<void(Project*)> = 0;
		virtual auto getLinkingExecFunc   ()         -> std::function<void(Project*)> = 0;
		virtual auto getCompileCppFileFunc()         -> std::function<void(std::string*)> = 0;
		virtual auto getCompileCppFileFuncDep()      -> std::function<void(std::string*)> = 0;
		virtual auto getCompileCFileFunc  ()         -> std::function<void(std::string*)> = 0;
		virtual auto getCompileClangCompleteFunc()   -> std::function<void(std::string*)> = 0;
	};
}
