#include "Project.h"


namespace {
	bool isStartingWith(std::string const& str, std::string const& start) {
		if (str.length() <= start.length()) {
			return false;
		}
		std::string sub = str.substr(0, start.length());

		return sub == start;
	}
}


namespace busyConfig {
/** tries to determine type by name
 *  name starting with "test" are executables
 *  everything else is a library
 */
auto Project::getDefaultTypeByName() const -> std::string {
	if (isStartingWith(name, "test")) {
		return "executable";
	}
	return "library";
}

}
