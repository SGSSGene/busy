#include "Project.h"

#include <busyUtils/busyUtils.h>

namespace busy::analyse {

namespace fs = std::filesystem;


Project::Project(std::string_view _name, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths) {
	analyseFiles(_name, _sourcePath, _legacyIncludePaths);
}

void Project::analyseFiles(std::string_view _name, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths) {
	mSourceFiles = {
		{FileType::C, {}},
		{FileType::Cpp, {}},
		{FileType::H, {}},
	};

	// Discover cpp and c files
	for (auto const &e : fs::recursive_directory_iterator(_sourcePath)) {
		if (not is_regular_file(e)) {
			continue;
		}
		auto ext = e.path().extension();
		if (ext == ".c") {
			mSourceFiles[FileType::C].emplace_back(e.path());
		} else if (ext == ".cpp") {
			mSourceFiles[FileType::Cpp].emplace_back(e.path());
		} else {
			mSourceFiles[FileType::H].emplace_back(e.path());
		}
	}

	//!TODO discover legacy include paths
	// Discover header files
//	for (auto const& dir : _legacyIncludePaths) {
//		for (auto const& f : utils::listFiles(dir, true)) {
//			mSourceFiles["incl"].push_back(dir + "/" + f);
//			mSourceFiles["incl-flat"].push_back(f);
//		}
//	}

}


}
