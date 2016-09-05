#include "PackageURL.h"

#include <busyUtils/busyUtils.h>

namespace busyConfig {
	void PackageURL::generateNameFromURL() {
		name = url;
		if (utils::isEndingWith(name, ".git")) {
			for (int i {0}; i<4; ++i) name.pop_back();
		}
		auto l = utils::explode(name, "/");
		if (l.size() == 1) {
			name = l[0];
		} else {
			name = l[l.size()-1];
		}
		l = utils::explode(name, ":");
		if (l.size() == 1) {
			name = l[0];
		} else {
			name = l[l.size()-1];
		}
	}

}
