#include "Override.h"

namespace busy {

auto Override::getProjectToOverride() const -> std::string const& {
	return projectToOverride;
}
auto Override::getExcludeFromToolchains() const -> std::vector<std::string> const& {
	return excludeFromToolchains;
}

}
