#pragma once

#include "Project.h"

#include <busyUtils/busyUtils.h>

#include <filesystem>
#include <set>
#include <string>
#include <string_view>

namespace busy::analyse {

class Package {
public:
	static constexpr std::string_view external{"external"};

private:
	std::filesystem::path mPath;
	std::string mName;
	std::vector<Project> mProjects;
	std::vector<Package> mPackages;
public:
	Package(std::filesystem::path const& _path);

	auto getPath() const -> std::filesystem::path {
		return mPath;
	}

	auto getName() const -> std::string const& {
		return mName;
	}

	auto getProjects() const -> std::vector<Project> const& {
		return mProjects;
	}

	auto getPackages() const -> std::vector<Package> const& {
		return mPackages;
	}

	auto isEquivalent(Package const& _other) const -> bool {
		if (mProjects.size() != _other.mProjects.size()) {
			return false;
		}
		if (mPackages.size() != _other.mPackages.size()) {
			return false;
		}
		for (int i{0}; i < mProjects.size(); ++i) {
			if (not mProjects[i].isEquivalent(_other.mProjects[i])) {
				return false;
			}
		}
		for (int i{0}; i < mPackages.size(); ++i) {
			if (not mPackages[i].isEquivalent(_other.mPackages[i])) {
				return false;
			}
		}
		return true;
	}
};

}
