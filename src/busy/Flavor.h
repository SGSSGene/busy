#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace busy {
	class Project;

	struct Flavor {
		std::string name;
		std::string buildMode; //! TODO should be a pointer to BuildMode
		std::string toolchain; //! TODO should be a pointer to the toolchain

		std::vector<std::string> mLinkAsShared;
		std::vector<std::string> mRPaths;
	};

}
