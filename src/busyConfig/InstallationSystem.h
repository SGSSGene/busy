#pragma once

#include <set>
#include <string>
#include <vector>

namespace busyConfig {

	struct InstallationSystem {
		std::set<std::string> systems;
		std::string type;
		std::string url;

		template <typename Node>
		void serialize(Node& node) {
			node["systems"] % systems;
			node["type"]    % type;
			node["url"]     % url;
		}
	};


}

