#pragma once

#include <filesystem>
#include <map>
#include <string_view>
#include <vector>

namespace busy::analyse {

enum class FileType {
	C,
	Cpp,
	H
};

class Project {
private:
	std::map<FileType, std::vector<std::filesystem::path>> mSourceFiles;

public:
	Project(std::string_view _name, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths);

	auto const& getSourceFiles() const {
		return mSourceFiles;
	}
private:
	void analyseFiles(std::string_view _name, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths);
};

}
