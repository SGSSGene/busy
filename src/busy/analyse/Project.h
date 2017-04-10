#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace busy {
namespace analyse {

class Project {
private:
	std::map<std::string, std::vector<std::string>> mSourceFiles;

public:
	Project(std::string const& _name, std::string const& _sourcePath, std::vector<std::string> const& _legacyIncludePaths);

	auto getSourceFiles() const -> std::map<std::string, std::vector<std::string>> const& {
		return mSourceFiles;
	}
private:
	void analyseFiles(std::string const& _name, std::string const& _sourcePath, std::vector<std::string> const& _legacyIncludePaths);
};

}
}

