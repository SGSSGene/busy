#pragma once

#include "Project.h"

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

	friend auto isEquivalent(Package const& l, Package const& r) -> bool {
		if (l.mProjects.size() != r.mProjects.size()) {
			return false;
		}
		if (l.mPackages.size() != r.mPackages.size()) {
			return false;
		}
		for (int i{0}; i < l.mProjects.size(); ++i) {
			if (not l.mProjects[i].isEquivalent(r.mProjects[i])) {
				return false;
			}
		}
		for (int i{0}; i < l.mPackages.size(); ++i) {
			if (not isEquivalent(l.mPackages[i], r.mPackages[i])) {
				return false;
			}
		}
		return true;
	}
};

}
