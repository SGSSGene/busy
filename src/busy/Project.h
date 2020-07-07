#pragma once

#include "File.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace busy {

class Project {
private:
	std::string mName;
	std::string mType;
	std::filesystem::path mPath;

	std::vector<File>                     mFiles;
	std::vector<std::filesystem::path>    mLegacyIncludePaths;
	std::set<std::string>                 mSystemLibraries;


public:
	Project(std::string _name, std::string _type, std::filesystem::path const& _root, std::filesystem::path _sourcePath, std::vector<std::filesystem::path> _legacyIncludePaths, std::set<std::string> _systemLibraries);

	auto const& getFiles() const {
		return mFiles;
	}

	auto getName() const -> std::string const& {
		return mName;
	}
	auto getType() const -> std::string const& {
		return mType;
	}

	auto getPath() const -> std::filesystem::path const& {
		return mPath;
	}

	auto getSystemLibraries() const -> std::set<std::string> const& {
		return mSystemLibraries;
	}

	auto getLegacyIncludePaths() const -> std::vector<std::filesystem::path> const& {
		return mLegacyIncludePaths;
	}

	auto getIncludes() const -> std::set<std::filesystem::path> {
		auto ret = std::set<std::filesystem::path>{};
		for (auto const& f : getFiles()) {
			for (auto const& i : f.getIncludes()) {
				ret.emplace(i);
			}
		}
		return ret;
	}

	auto isEquivalent(Project const& _other) const -> bool {
		if (mName != _other.mName) {
			return false;
		}
		if (mFiles.size() != _other.mFiles.size()) {
			return false;
		}
		if (mSystemLibraries != _other.mSystemLibraries) {
			return false;
		}


		auto hasSameFile = [&](auto const& _file) {
			for (auto const& f : _other.mFiles) {
				if (f.isEquivalent(_file)) {
					return true;
				}
			}
			return false;
		};


		for (auto const& file : mFiles) {
			if (not hasSameFile(file)) {
				return false;
			}
		}
		return true;
	}

private:
	void analyseFiles(std::string const& _name, std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths);
};

}
