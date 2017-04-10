#include "Project.h"

#include <busyUtils/busyUtils.h>

namespace busy {
namespace analyse {

Project::Project(std::string const& _name, std::string const& _sourcePath, std::vector<std::string> const& _legacyIncludePaths) {
	analyseFiles(_name, _sourcePath, _legacyIncludePaths);
}

void Project::analyseFiles(std::string const& _name, std::string const& _sourcePath, std::vector<std::string> const& _legacyIncludePaths) {
	mSourceFiles["cpp"]       = {};
	mSourceFiles["c"]         = {};
	mSourceFiles["incl"]      = {};
	mSourceFiles["incl-flat"] = {};

	// Discover cpp and c files
	for (auto const &f : utils::listFiles(_sourcePath, true)) {
		if (utils::isEndingWith(f, ".cpp")) {
			mSourceFiles["cpp"].push_back(_sourcePath + "/" + f);
		} else if (utils::isEndingWith(f, ".c")) {
			mSourceFiles["c"].push_back(_sourcePath + "/" + f);
		} else {
			mSourceFiles["incl"].push_back(_sourcePath + "/" + f);
			mSourceFiles["incl-flat"].push_back(_name + "/" + f);
		}
	}

	// Discover header files
	for (auto const& dir : _legacyIncludePaths) {
		for (auto const& f : utils::listFiles(dir, true)) {
			mSourceFiles["incl"].push_back(dir + "/" + f);
			mSourceFiles["incl-flat"].push_back(f);
		}
	}

}


}
}
