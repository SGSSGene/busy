#pragma once

#include <busyConfig/busyConfig.h>

namespace busy {
	struct Flavor {
		std::string buildMode;
		std::string toolchain;

		Flavor() {}
		Flavor(busyConfig::Flavor const& _flavor) {
			buildMode = _flavor.buildMode;
			toolchain = _flavor.toolchain;
		}
	};
}

