#pragma once

#include <string>
#include <vector>

namespace busyConfig {
	struct ProjectLegacy {
		std::vector<std::string> includes;
		std::vector<std::string> systemIncludes;
		std::vector<std::string> systemLibraries;
		std::vector<std::string> linkingOption;

		template <typename Node>
		void serialize(Node& node) {
			node["includes"]        % includes;
			node["systemIncludes"]  % systemIncludes;
			node["systemLibraries"] % systemLibraries;
			node["linkingOption"]   % linkingOption;
		}
	};
}
