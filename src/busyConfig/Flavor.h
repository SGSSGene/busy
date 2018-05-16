#pragma once

#include <string>
#include <vector>

namespace busyConfig {
	struct Flavor {
		std::string buildMode;
		std::string toolchain;
		std::vector<std::string> linkAsShared;
		std::vector<std::string> rpath;

		template <typename Node>
		void serialize(Node& node) {
			node["buildMode"] % buildMode;
			node["toolchain"] % toolchain;
			node["linkAsShared"] % linkAsShared;
			node["rpath"]        % rpath;
		}
	};
}

