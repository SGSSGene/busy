#pragma once

#include <set>
#include <string>
#include <vector>

namespace busyConfig {

	struct Override {
		std::string              projectToOverride;
		std::vector<std::string> excludeFromToolchains;

		template <typename Node>
		void serialize(Node& node) {
			node["projectToOverride"]     % projectToOverride;
			node["excludeFromToolchains"] % excludeFromToolchains;
		}
	};
}
