#pragma once

#include <string>
#include <vector>

namespace busyConfig {

	struct Toolchain {
		std::string name;

		struct Command {
			std::vector<std::string> command;
			std::vector<std::string> postOptions;

			template <typename Node>
			void serialize(Node& node) {
				node["command"]     % command;
				node["postOptions"] % postOptions;
			}
		};


		Command cCompiler;
		Command cppCompiler;
		Command archivist;
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
