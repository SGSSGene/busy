#pragma once

#include <map>
#include <string>
#include <vector>

namespace busyConfig {

	struct Toolchain {
		std::string name;
		std::string version;

		struct Command {
			using Flags = std::vector<std::string>;
			using BuildModes = std::map<std::string, Flags>;

			std::vector<std::string> searchPaths;
			std::vector<std::string> strict;
			std::vector<std::string> flags;
			std::vector<std::string> flags2;
			BuildModes               buildModeFlags;

			template <typename Node>
			void serialize(Node& node) {
				node["searchPaths"]    % searchPaths;
				node["strict"]         % strict;
				node["flags"]          % flags;
				node["flags2"]         % flags2;
				node["buildModeFlags"] % buildModeFlags;
			}
		};

		Command cCompiler;
		Command cppCompiler;
		Command linkExecutable;
		Command archivist;

		template <typename Node>
		void serialize(Node& node) {
			node["name"]           % name;
			node["version"]        % version;
			node["ccompiler"]      % cCompiler;
			node["cppcompiler"]    % cppCompiler;
			node["linkExecutable"] % linkExecutable;
			node["archivist"]      % archivist;
		}

	};

}
