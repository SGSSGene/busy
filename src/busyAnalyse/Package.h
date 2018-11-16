#pragma once

#include "Project.h"

#include <busyConfig2/busyConfig.h>
#include <busyUtils/busyUtils.h>

#include <filesystem>
#include <set>
#include <string>

namespace busy::analyse {

class Package {
private:
	std::filesystem::path mPath;
	std::string mName;
	std::vector<Project> mProjects;
	std::vector<Package> mPackages;
public:
	Package(std::filesystem::path const& _path)
		: mPath { _path.lexically_normal() }
	{
		// read this package config
		auto package = ::busyConfig::readPackage(mPath);
		mName = package.name;

		// load external packages
		namespace fs = std::filesystem;
		// adding entries to package
		if (fs::status(mPath / "external").type() == fs::file_type::directory) {
			for(auto& p : fs::directory_iterator(mPath / "external")) {
				mPackages.push_back(p.path());
			}
		}

		// load projects
		for(auto& p : fs::directory_iterator(mPath / "src")) {
			if (not fs::is_directory(p)) {
				continue;
			}
			auto name = p.path().filename();
			mProjects.emplace_back(Project{name, p.path(), {}});
		}
	}

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
