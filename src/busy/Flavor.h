#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace busy {
	class Project;

	struct Flavor {
		std::string buildMode; //! TODO should be a pointer to BuildMode
		std::string toolchain; //! TODO should be a pointer to the toolchain

		std::vector<std::string> mLinkAsSharedAsStrings;
		std::vector<Project const*> mLinkAsShared; //! modules should be shared libraries

		bool isShared(Project const* _project) const {
			return std::find(mLinkAsShared.begin(), mLinkAsShared.end(), _project) != mLinkAsShared.end();
		}
	};

}
