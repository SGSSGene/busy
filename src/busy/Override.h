#pragma once

#include <busyConfig/busyConfig.h>

namespace busy {

	class Override {
	private:
		std::string              projectToOverride;
		std::vector<std::string> excludeFromToolchains;
	public:
		Override() {}
		Override(busyConfig::Override const& _override);

		auto getProjectToOverride() const -> std::string const&;
		auto getExcludeFromToolchains() const -> std::vector<std::string> const&;

	};

}
