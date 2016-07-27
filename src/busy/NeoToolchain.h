#pragma once

#include <string>
#include <vector>

namespace busy {

struct NeoToolchain {
	std::vector<std::string> cCompiler;
	std::vector<std::string> cppCompiler;
	std::vector<std::string> archivist;
};

}
