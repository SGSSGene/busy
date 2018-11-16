#pragma once

#include <string>
#include <vector>

namespace busyConfig {
	struct ProjectLegacy {
		std::vector<std::string> includes;
		std::vector<std::string> systemIncludes;
		std::vector<std::string> systemLibraries;
		std::vector<std::string> linkingOption;
		std::vector<std::string> dependentProjects;
		std::vector<std::string> depLibraries;
		std::vector<std::string> defines;
		bool                     dependencyDiscovery { true };
		bool                     wholeArchive { false };


		template <typename Node>
		void serialize(Node& node) {
			node["dependentProjects"]   % dependentProjects;
			node["includes"]            % includes;
			node["systemIncludes"]      % systemIncludes;
			node["systemLibraries"]     % systemLibraries;
			node["linkingOption"]       % linkingOption;
			node["depLibraries"]        % depLibraries;
			node["defines"]             % defines;
			node["dependencyDiscovery"] % dependencyDiscovery;
			node["wholeArchive"]        % wholeArchive;
		}
	};
}
