#pragma once

#include "Installation.h"
#include "Override.h"
#include "PackageURL.h"
#include "Project.h"
#include "Toolchain.h"
#include "Flavor.h"

#include <map>

namespace busyConfig {

	using ExtRepositories = std::vector<PackageURL>;
	using Projects        = std::vector<Project>;
	using Installations   = std::vector<Installation>;
	using Toolchains      = std::vector<Toolchain>;
	using Overrides       = std::vector<Override>;
	using Flavors         = std::map<std::string, Flavor>;

	struct Package {
		std::string     name;
		bool            gitAnnex;
		ExtRepositories extRepositories;
		Projects        projects;
		Overrides       overrides;
		Installations   installations;
		Toolchains      toolchains;
		Flavors         flavors;

		template <typename Node>
		void serialize(Node& node) {
			node["name"]            % name;
			node["git-annex"]       % gitAnnex;
			node["extRepositories"] % extRepositories;
			node["projects"]        % projects;
			node["overrides"]       % overrides;
			node["installations"]   % installations;
			node["toolchains"]      % toolchains;
			node["flavors"]         % flavors;
		}

	};
}
