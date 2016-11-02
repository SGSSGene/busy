#pragma once

#include "FileStat.h"
#include <string>
#include <vector>


namespace busy {
	struct WorkspaceConfig {
		std::map<std::string, FileStat>    mFileStats;
		std::string                        mToolchainName;
		std::string                        mBuildModeName;
		std::vector<std::string>           mStaticAsShared;
		std::vector<std::string>           mRPaths;

		template <typename Node>
		void serialize(Node& node) {
			node["fileStats"]      % mFileStats;
			node["toolchainName"]  % mToolchainName;
			node["buildModeName"]  % mBuildModeName;
			node["staticAsShared"] % mStaticAsShared;
			node["rpaths"]         % mRPaths;
		}
	};
}
