#include "Project.h"


namespace {
	bool hasPrefix(std::string_view str, std::string_view prefix) {
		if (str.length() < prefix.length()) {
			return false;
		}
		for (size_t i{0}; i < prefix.length(); ++i) {
			if (str.at(i) != prefix.at(i)) {
				return false;
			}
		}
		return true;
	}
}


namespace busyConfig {
/** tries to determine type by name
 *  name starting with "test" are executables
 *  everything else is a library
 */
auto Project::getDefaultTypeByName() const -> std::string {
	for (auto prefix : {"test", "demo", "example"}) {
		if (hasPrefix(name, prefix)) {
			return "executable";
		}
	}
	return "library";
}

}
