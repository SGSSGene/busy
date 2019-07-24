#include "Project.h"

namespace busy::analyse {

Project::Project(std::string _name, std::filesystem::path const& _root, std::filesystem::path _sourcePath, std::vector<std::filesystem::path> _legacyIncludePaths, std::set<std::string> _systemLibraries)
	: mName               { std::move(_name) }
	, mPath               { _sourcePath.lexically_normal() }
	, mLegacyIncludePaths { std::move(_legacyIncludePaths) }
	, mSystemLibraries    { std::move(_systemLibraries) }
{
	analyseFiles(mName, _root, mPath, mLegacyIncludePaths);
}

void Project::analyseFiles(std::string const& _name, std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths) {
	mFiles = {
		{FileType::C, {}},
		{FileType::Cpp, {}},
		{FileType::H, {}},
	};

	// Discover header, cpp and c files
	for (auto const &e : std::filesystem::recursive_directory_iterator(_root / _sourcePath)) {
		if (not is_regular_file(e)) {
			continue;
		}
		auto ext = e.path().extension();
		auto path = e.path();

		auto fileType = [&]() {
			if (ext == ".c") return FileType::C;
			if (ext == ".cpp") return FileType::Cpp;
			return FileType::H;
		}();

		mFiles[fileType].emplace_back(_root, relative(path, _root).lexically_normal());
	}

	// Discover legacy include paths
	for (auto const& dir : _legacyIncludePaths) {
		if (not is_directory(_root / dir)) {
			continue;
		}
		for (auto const &e : std::filesystem::recursive_directory_iterator(_root / dir)) {
			if (not is_regular_file(e)) {
				continue;
			}
			auto path = relative(e.path(), _root).lexically_normal();
			mFiles[FileType::H].emplace_back(_root, std::move(path));
		}
	}
}

}
