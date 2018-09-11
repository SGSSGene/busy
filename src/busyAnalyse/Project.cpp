#include "Project.h"

#include <busyUtils/busyUtils.h>

namespace busy {
namespace analyse {

Project::Project(std::string _name, std::string _sourcePath, std::vector<std::string> const& _legacyIncludePaths)
	: mName { std::move(_name) }
	, mPath { std::move(_sourcePath) }
{
	analyseFiles(mName, mPath, _legacyIncludePaths);
}

void Project::analyseFiles(std::string const& _name, std::string const& _sourcePath, std::vector<std::string> const& _legacyIncludePaths) {
	mFiles["cpp"]       = {};
	mFiles["c"]         = {};
	mFiles["incl"]      = {};

	// Discover cpp and c files
	for (auto const &f : utils::listFiles(_sourcePath, true)) {
		auto path     = _sourcePath + "/" + f;
		auto flatPath = _name + "/" + f;

		auto type = [&]() -> std::string {
			if (utils::isEndingWith(f, ".cpp")) {
				return "cpp";
			} else if (utils::isEndingWith(f, ".c")) {
				return "c";
			}
			return "incl";
		}();

		mFiles[type].push_back({path, flatPath});
	}

	// Discover header files
	for (auto const& dir : _legacyIncludePaths) {
		for (auto const& f : utils::listFiles(dir, true)) {
			mFiles["incl"].push_back({dir + "/" + f, f});
		}
	}

}


}
}
