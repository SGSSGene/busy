#include "Project.h"

#include "Workspace.h"
#include "Package.h"

#include "FileStates.h"
#include <algorithm>
#include <iostream>
#include <fstream>
namespace busy {

Project::Project(busyConfig::Project const& _project) {
	path                       = _project.name;
	dependencies               = _project.dependencies;
	optionalDependencies       = _project.optionalDependencies;
	type                       = _project.type;
	legacy                     = _project.legacy;
	depLibraries               = _project.depLibraries;
	defines                    = _project.defines;
	noWarnings                 = _project.noWarnings;
	wholeArchive               = _project.wholeArchive;
	mAutoDependenciesDiscovery = _project.mAutoDependenciesDiscovery;
	mIgnore                    = _project.mIgnore;
	linkAsShared               = _project.linkAsShared;

}

void Project::setWorkspace(NeoWorkspace* _workspace) {
	mWorkspace = _workspace;
}



void Project::quickFix() {
//!TODO sometimes wrong dependencies get detected, this is bad
/*	for (auto const& d : getDefaultDependencies()) {
		if (std::find(dependencies.begin(), dependencies.end(), d) == dependencies.end()) {
			dependencies.push_back(d);
		}
	}*/
	std::sort(dependencies.begin(), dependencies.end());
}

auto Project::getSourcePaths() const -> std::vector<std::string> {
	std::vector<std::string> sourcePaths;
	sourcePaths.emplace_back(getPath());
	std::sort(sourcePaths.begin(), sourcePaths.end());
	return sourcePaths;
}
auto Project::getIncludePaths() const -> std::vector<std::string> {
	std::vector<std::string> includePaths;

	includePaths.emplace_back(getPackagePath() + "/src/" + getName());
	for (auto const& i : getLegacy().includes) {
		includePaths.emplace_back(getPackagePath() + "/" + i + "/");
	}
	std::sort(includePaths.begin(), includePaths.end());
	return includePaths;
}


/** tries to determine type by name
 *  name starting with "test" are executables
 *  everything else is a library
 */
auto Project::getDefaultTypeByName() const -> std::string {
	if (utils::isStartingWith(path, "test")) {
		return "executable";
	}
	return "library";
}

namespace {
	void skipAllWhiteSpaces(char const*& str) {
		while (*str != '\0' and (*str == '\t' or *str == ' ')) ++str;
	}
	bool checkIfMakroEndif(char const* str) {
		skipAllWhiteSpaces(str);
		return strncmp(str, "#endif", 6) == 0;
	}
	bool checkIfMakroIfAndBusy(char const* str) {
		skipAllWhiteSpaces(str);
		if (strncmp(str, "#if", 3) != 0) {
			return false;
		}
		str += 3;
		skipAllWhiteSpaces(str);
		return strncmp(str, "BUSY_", 5) == 0;
	}
	bool checkIfMakroSystemInclude(char const* str) {
		skipAllWhiteSpaces(str);
		if (strncmp(str, "#include", 8) != 0) {
			return false;
		}
		str += 8;
		skipAllWhiteSpaces(str);
		return *str == '<';
	}
}

auto Project::getDefaultDependencies(Workspace* _workspace, std::map<std::string, Project> const& _projects) const -> Dependencies {
	getDefaultAndOptionalDependencies(_workspace, _projects);
	return mCachedDefaultDependencies;
}
auto Project::getDefaultOptionalDependencies(Workspace* _workspace, std::map<std::string, Project> const& _projects) const -> Dependencies {
	getDefaultAndOptionalDependencies(_workspace, _projects);
	return mCachedOptionalDependencies;
}

void Project::getDefaultAndOptionalDependencies(Workspace* _workspace, std::map<std::string, Project> const& _projects) const {
	if (mCachedDependenciesValid) return;

	auto& fileStates = _workspace->accessConfigFile().accessAutoFileStates();
	Dependencies dep;
	auto allFiles = getAllFiles({".cpp", ".c", ".h", ".hpp"});
	std::set<std::tuple<bool, std::string>> filesOfInterest;
	for (auto const& f : allFiles) {
		auto currentTime = getFileStates().getFileModTime(f);
		auto lastTime    = fileStates[f].lastChange;
		if (lastTime == currentTime and not fileStates[f].hasChanged) {
			for (auto const& file : fileStates[f].dependencies) {
				filesOfInterest.insert(std::make_tuple(false, file));
			}
			for (auto const& file : fileStates[f].optDependencies) {
				filesOfInterest.insert(std::make_tuple(true, file));
			}
			continue;
		}
		fileStates[f].lastChange = currentTime;
		fileStates[f].hasChanged = true;

		std::ifstream ifs(f);
		std::string line;

		bool optionalSection = false;

		while (std::getline(ifs, line)) {
			// check if it is '#endif'
			if (checkIfMakroEndif(line.c_str())) {
				optionalSection = false;
			} else if (checkIfMakroIfAndBusy(line.c_str())) {
				optionalSection = true;
			} else if (checkIfMakroSystemInclude(line.c_str())) {
				auto parts = utils::explode(line, std::vector<std::string>{" ", "\t"});

				std::string include;
				if (parts.size() == 1) {
					include = parts[0];
				} else {
					include = parts[1];
				}

				auto pos1 = include.find("<")+1;
				auto pos2 = include.find(">")-pos1;

				auto file = include.substr(pos1, pos2);
				filesOfInterest.insert(std::make_tuple(optionalSection, file));
				fileStates[f].dependencies.push_back(file);
			}
		}
	}

	for (auto const& tuple : filesOfInterest) {
		bool optional;
		std::string file;
		std::tie(optional, file) = tuple;

		for (auto const& e : _projects) {
			auto& project = e.second;
			if (&project == this) continue;

			for (auto const& h : project.getAllHFilesFlat()) {
				if (file == h) {
					auto package = project.getPackagePath();
					auto pList   = utils::explode(package, "/");
					auto d       = pList.back() + "/" + project.getName();
					
					auto* dep = &mCachedDefaultDependencies;
					if (optional) {
						dep = &mCachedOptionalDependencies;
					}

					if (std::find(dep->begin(), dep->end(), d) == dep->end()) {
						dep->emplace_back(d);
					}
					break;
				}
			}
		}
	}
	mCachedDependenciesValid = true;
}

auto Project::getAllFiles(std::set<std::string> const& _ending) const -> std::vector<std::string> {
	std::vector<std::string> files;

	for (auto const& p : getIncludePaths()) {
		if (not utils::dirExists(p)) continue;

		auto allFiles = utils::listFiles(p, true);
		for (auto const& f : allFiles) {
			for (auto const& ending : _ending) {
				if (utils::isEndingWith(f, ending)) {
					files.push_back(p + "/" + f);
					break;
				}
			}
		}
	}
	return files;
}
auto Project::getAllFilesFlat(std::set<std::string> const& _ending, bool noending) const -> std::vector<std::string> {
	std::vector<std::string> files;

	std::vector<std::tuple<std::string, std::string>> allPaths;
	allPaths.emplace_back(std::make_tuple(std::string("./") + getPackagePath() + "/src/" + getPath() + "/", getPath() + "/"));
	for (auto const& i : getLegacy().includes) {
		allPaths.emplace_back(std::make_tuple(std::string("./") + getPackagePath() + "/" + i + "/", std::string("")));
	}

	for (auto const& p : allPaths) {
		if (not utils::dirExists(std::get<0>(p))) continue;

		auto allFiles = utils::listFiles(std::get<0>(p), true);
		for (auto const& f : allFiles) {
			if (noending and f.find(".") == std::string::npos) {
				files.push_back(std::get<1>(p) + f);
				continue;
			}
			for (auto const& ending : _ending) {
				if (utils::isEndingWith(f, ending)) {
					files.push_back(std::get<1>(p) + f);
					break;
				}
			}
		}
	}
	return files;
}
auto Project::getAllFilesFlatNoEnding() const -> std::vector<std::string> {
	std::vector<std::string> files;

	std::vector<std::tuple<std::string, std::string>> allPaths;
	allPaths.emplace_back(std::make_tuple(std::string("./") + getPackagePath() + "/src/" + getPath() + "/", getPath() + "/"));
	for (auto const& i : getLegacy().includes) {
		allPaths.emplace_back(std::make_tuple(std::string("./") + getPackagePath() + "/" + i + "/", std::string("")));
	}

	for (auto const& p : allPaths) {
		if (not utils::dirExists(std::get<0>(p))) continue;

		auto allFiles = utils::listFiles(std::get<0>(p), true);
		for (auto const& f : allFiles) {
			if (f.find('.') == std::string::npos) {
				files.push_back(std::get<1>(p) + f);
				break;
			}
		}
	}
	return files;
}




auto Project::getAllCppFiles() -> std::vector<std::string>& {
	if (cppFiles.empty()) {
		cppFiles = getAllFiles({".cpp"});
		std::sort(cppFiles.begin(), cppFiles.end());
	}
	return cppFiles;
}
auto Project::getAllCFiles() -> std::vector<std::string>& {
	if (cFiles.empty()) {
		cFiles = getAllFiles({".c"});
		std::sort(cFiles.begin(), cFiles.end());
	}
	return cFiles;
}
auto Project::getAllHFiles() const -> std::vector<std::string> const& {
	if (hFiles.empty()) {
		hFiles = getAllFiles({".h", ".hpp"});
		std::sort(hFiles.begin(), hFiles.end());
	}

	return hFiles;
}
auto Project::getAllHFilesFlat() const -> std::vector<std::string> const& {
	if (hFilesFlat.empty()) {
		hFilesFlat = getAllFilesFlat({".h", ".hpp"}, true);
		std::sort(hFilesFlat.begin(), hFilesFlat.end());
	}

	return hFilesFlat;
}



auto Project::getComIncludePaths() const -> std::vector<std::string> {
	std::vector<std::string> retList;

	// add all includes from legacy
	for (auto const& i : getLegacy().includes) {
		retList.emplace_back(getPackagePath() + "/" + i);
	}

	// add all local includes
	retList.emplace_back(getPackagePath()+"/src/" + getPath());

	// add itself as inlude
	retList.emplace_back(getPackagePath()+"/src/");

	return retList;

}
auto Project::getComSystemIncludePaths(std::set<Project*> const& _dependencies) const -> std::vector<std::string> {
	std::vector<std::string> retList;

	// Adding all includes of dependent libraries
	for (auto const& project : _dependencies) {
		retList.push_back(project->getPackagePath()+"/src");
		for (auto const& i : project->getLegacy().includes) {
			retList.push_back(project->getPackagePath()+"/"+i);
		}
		for (auto const& i : project->getLegacy().systemIncludes) {
			retList.push_back(i);
		}
	}

	return retList;
}

auto Project::getComDefines(std::set<Project*> const& _dependencies) const -> std::vector<std::string> {
	std::vector<std::string> retList;

	// Adding macro to indicate busy is being used
	retList.push_back("-DBUSY");

	// Adding all defines of dependend libraries
	for (auto const& project : _dependencies) {
		std::string def = std::string("-DBUSY_");
		for (auto const& c : project->getName()) {
			if ((c >= 'A' and c <= 'Z')
			    or (c >= 'a' and c <= 'z')
			    or (c >= '0' and c <= '9')) {
				def += std::toupper(c);
			} else {
				def += "_";
			}
		}
		retList.push_back(def);
	}

	// Adding library itself to busy definition
	{
		std::string def = std::string("-DBUSY_");
		for (auto const& c : getName()) {
			if ((c >= 'A' and c <= 'Z')
			    or (c >= 'a' and c <= 'z')
			    or (c >= '0' and c <= '9')) {
				def += std::toupper(c);
			} else {
				def += "_";
			}
		}
		retList.push_back(def);
	}

	return retList;
}




}

