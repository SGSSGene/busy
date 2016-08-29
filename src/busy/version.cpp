#include "version.h"
#include <busy-version/version.h>

namespace busy {
	auto version() -> std::string {
		return VERSION_BUSY;
	}
}
