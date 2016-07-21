#pragma once

#include "Flavor.h"

namespace busyConfig {
	struct Flavor {
		std::string buildMode;
		std::string toolchain;

		template <typename Node>
		void serialize(Node& node) {
			node["buildMode"] % buildMode;
			node["toolchain"] % toolchain;
		}
	};
}

