#pragma once

#include "Flavor.h"

namespace busyConfig {
	struct Flavor {
		std::string buildMode;
		std::string toolchain;
		std::vector<std::string> linkAsShared;

		template <typename Node>
		void serialize(Node& node) {
			node["buildMode"] % buildMode;
			node["toolchain"] % toolchain;
			node["linkAsShared"] % linkAsShared;
		}
	};
}

