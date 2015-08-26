#pragma once

#include "graph.h"

namespace aBuild {
	class BuildAction {
	protected:
		Graph const* graph;
		const bool verbose;
	public:
		BuildAction(Graph const* _graph, bool _verbose)
			: graph   {_graph}
			, verbose {_verbose}
			{}
		virtual auto getLinkingLibFunc    () -> std::function<void(Project*)> = 0;
		virtual auto getLinkingExecFunc   () -> std::function<void(Project*)> = 0;
		virtual auto getCompileCppFileFunc() -> std::function<void(std::string*)> = 0;
		virtual auto getCompileCFileFunc  () -> std::function<void(std::string*)> = 0;
	};
}
