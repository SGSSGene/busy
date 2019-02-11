#pragma once

#include <map>
#include <string>
#include <vector>

namespace busyConfig {

struct Toolchain {
	std::string name;
	std::string version;

	struct Command {
		std::vector<std::string> searchPaths;
		std::vector<std::string> call;

		template <typename Node>
		constexpr void serialize(Node& node) {
			node["searchPaths"]    % searchPaths;
			node["call"]           % call;
		}
	};

	Command cCompiler;
	Command cppCompiler;
	Command linkExecutable;
	Command archivist;

	template <typename Node>
	constexpr void serialize(Node& node) {
		node["name"]           % name;
		node["version"]        % version;
		node["ccompiler"]      % cCompiler;
		node["cppcompiler"]    % cppCompiler;
		node["linkExecutable"] % linkExecutable;
		node["archivist"]      % archivist;
	}
};

}
