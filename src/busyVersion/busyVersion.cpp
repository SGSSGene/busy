#include "busyVersion.h"
#include <busy-version/version.h>

namespace busyVersion {
	auto version() -> std::string {
		return VERSION_BUSY;
	}

}
