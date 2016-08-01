#pragma once

#include <set>
#include <string>
#include <vector>

namespace busy {
	struct Override {
		std::string project;
		std::set<std::string> toolchains;
	};
	using Overrides = std::vector<Override>;
}
