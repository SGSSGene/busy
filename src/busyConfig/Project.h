#pragma once

#include "ProjectLegacy.h"


namespace busyConfig {

	struct Project {
		std::string              name;
		std::string              type {"library"};
		ProjectLegacy            legacy;

		template <typename Node>
		void serialize(Node& node) {
			node["name"]                      % name;
			node["type"]                      % type;
			node["legacy"]                    % legacy;
		}
	private:
		auto getDefaultTypeByName() const -> std::string;
	};
}
