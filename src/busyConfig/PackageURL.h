#pragma once

#include <string>

namespace busyConfig {

	struct PackageURL {
		std::string name;
		std::string url;
		std::string branch;

		template <typename Node>
		void serialize(Node& node) {
			node["url"]    % url;
			node["branch"] % branch or std::string("master");

			generateNameFromURL();
		}

		void generateNameFromURL();
	};

}
