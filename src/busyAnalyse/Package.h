#pragma once

#include <busyConfig/busyConfig.h>
#include <busyUtils/busyUtils.h>

#include <set>
#include <string>

#include "Project.h"

namespace busy {
namespace analyse {

class Package {
private:
	std::string mPath;
	std::string mName;
	std::vector<Project> mProjects;
	std::vector<Package> mPackages;
public:
	Package(std::string _path)
		: mPath { std::move(_path) }
	{
		// read this package config
		auto package = ::busyConfig::readPackage(mPath);
		mName = package.name;

		// load external packages
		auto extPath = mPath + "external/";
		if (::utils::fileExists(extPath)) {
			auto packagesDirs = ::utils::listDirs(extPath, true);
			for (auto const& p : packagesDirs) {
				mPackages.emplace_back(extPath + p + "/");
			}
		}

		// load projects
		auto projectDirs = ::utils::listDirs(mPath + "src/", true);
		for (auto const& p : projectDirs) {
			mProjects.emplace_back(Project{p, mPath + "src/" + p, {}});
		}
	}

	auto getPath() const -> std::string const& {
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
}

