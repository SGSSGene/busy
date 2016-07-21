#pragma once

#include "InstallationSystem.h"

namespace busyConfig {

	struct Installation {
		std::string name;
		std::vector<InstallationSystem> systems;

		template <typename Node>
		void serialize(Node& node) {
			node["name"]    % name;
			node["systems"] % systems;
		}
	};


}

