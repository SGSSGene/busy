#pragma once

#include "File.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace busy::analyse {

enum class FileType {
	C,
	Cpp,
	H
};

class Project {
private:
	std::string mName;
	std::filesystem::path mPath;

	std::map<FileType, std::vector<File>> mFiles;
	std::vector<std::filesystem::path>    mLegacyIncludePaths;
	std::set<std::string>                 mSystemLibraries;


public:
	Project(std::string _name, std::filesystem::path const& _root, std::filesystem::path _sourcePath, std::vector<std::filesystem::path> _legacyIncludePaths, std::set<std::string> _systemLibraries);

	auto const& getFiles() const {
		return mFiles;
	}

	auto getName() const -> std::string const& {
		return mName;
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
		for (auto const& [key, value] : getFiles()) {
			for (auto const& f : value) {
				for (auto const& i : f.getIncludes()) {
					ret.emplace(i);
				}
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

		for (auto const& e1 : mFiles) {
			if (_other.mFiles.count(e1.first) == 0) {
				return false;
			}
			auto const& e2 = *_other.mFiles.find(e1.first);
			if (e2.second.size() != e1.second.size()) {
				return false;
			}
			for (int i{0}; i < e2.second.size(); ++i) {
				if (not e1.second[i].isEquivalent(e2.second[i])) {
					return false;
				}
			}
		}
		return true;
	}

private:
	void analyseFiles(std::string const& _name, std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths);
};

}
