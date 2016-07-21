#include "Override.h"

namespace busy {

Override::Override(busyConfig::Override const& _override) {
	projectToOverride     = _override.projectToOverride;
	excludeFromToolchains = _override.excludeFromToolchains;
}


auto Override::getProjectToOverride() const -> std::string const& {
	return projectToOverride;
}
auto Override::getExcludeFromToolchains() const -> std::vector<std::string> const& {
	return excludeFromToolchains;
}

}
