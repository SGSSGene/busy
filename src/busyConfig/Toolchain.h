#pragma once

#include <string>
#include <vector>

namespace busyConfig {

	struct Toolchain {
		std::string name;

		std::vector<std::string> cCompiler;
		std::vector<std::string> cppCompiler;
		std::vector<std::string> archivist;
		std::vector<std::string> installations;

		template <typename Node>
		void serialize(Node& node) {
			node["name"]          % name;
			node["ccompiler"]     % cCompiler;
			node["cppcompiler"]   % cppCompiler;
			node["archivist"]     % archivist;
			node["installations"] % installations;
		}

	};

}
