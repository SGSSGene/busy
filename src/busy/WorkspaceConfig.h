#pragma once

#include "FileStat.h"
#include <string>
#include <vector>


namespace busy {
	struct WorkspaceConfig {
		std::map<std::string, FileStat> mFileStats;
		std::string                     mToolchainName;
		std::string                     mBuildModeName;

		template <typename Node>
		void serialize(Node& node) {
			node["fileStats"]     % mFileStats;
			node["toolchainName"] % mToolchainName;
			node["buildModeName"] % mBuildModeName;
		}
	};
}
