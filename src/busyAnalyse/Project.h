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

public:
	Project(std::string _name, std::filesystem::path _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths);

	auto const& getFiles() const {
		return mFiles;
	}

	auto getName() const -> std::string const& {
		return mName;
	}

	auto getPath() const -> std::filesystem::path const& {
		return mPath;
	}

	auto isEquivalent(Project const& _other) const -> bool {
		if (mName != _other.mName) {
			return false;
		}
		if (mFiles.size() != _other.mFiles.size()) {
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
	void analyseFiles(std::string const& _name, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths);
};

}
