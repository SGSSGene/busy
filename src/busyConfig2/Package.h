#pragma once

#include "Project.h"
#include "Toolchain.h"

#include <map>

namespace busyConfig {

	using Packages        = std::vector<std::string>;
	using Projects        = std::vector<Project>;
	using Toolchains      = std::vector<Toolchain>;

	struct Package {
		std::string     name;
		Packages        packages;
		Projects        projects;
		Toolchains      toolchains;

		static constexpr std::string_view external{"external"};

		template <typename Node>
		void serialize(Node& node) {
			node["name"]            % name;
			node["projects"]        % projects;
			node["toolchains"]      % toolchains;
		}

	};
}
