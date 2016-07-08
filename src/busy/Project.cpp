#include "Project.h"

#include "Workspace.h"

#include "FileStates.h"
#include <algorithm>
#include <iostream>
#include <fstream>
namespace aBuild {

void Project::quickFix() {
//!TODO sometimes wrong dependencies get detected, this is bad
/*	for (auto const& d : getDefaultDependencies()) {
		if (std::find(dependencies.begin(), dependencies.end(), d) == dependencies.end()) {
			dependencies.push_back(d);
		}
	}*/
	std::sort(dependencies.begin(), dependencies.end());
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

auto Project::getDefaultDependencies(Workspace* _workspace, std::map<std::string, Project> const& _projects) const -> Dependencies {
	auto& fileStates = _workspace->accessConfigFile().accessAutoFileStates();
	Dependencies dep;
	auto allFiles = getAllFiles({".cpp", ".c", ".h", ".hpp"});
	std::set<std::string> filesOfInterest;
	for (auto const& f : allFiles) {
		auto currentTime = getFileStates().getFileModTime(f);
		auto lastTime    = fileStates[f].lastChange;
		if (lastTime == currentTime and not fileStates[f].hasChanged) {
			for (auto const& file : fileStates[f].dependencies) {
				filesOfInterest.insert(file);
			}
			continue;
		}
		fileStates[f].lastChange = currentTime;
		fileStates[f].hasChanged = true;

		std::ifstream ifs(f);
		std::string line;

		bool optionalSection = false;

		while (std::getline(ifs, line)) {
			auto parts = utils::explode(line, std::vector<std::string>{" ", "\t"});
			if (parts.size() == 1 && parts[0] == "#endif") {
				optionalSection = false;
			} else if (parts.size() == 2 && parts[0] == "#ifdef" && parts[1].substr(0, 7) == "BUSY_") {
				optionalSection = true;
			} else if (not optionalSection
			    && parts.size() == 2 && parts[0] == "#include"
			    && parts[1].front() == '<' && parts[1].back() == '>') {

				auto pos1 = parts[1].find("<")+1;
				auto pos2 = parts[1].find(">")-pos1;

				auto file = parts[1].substr(pos1, pos2);
				filesOfInterest.insert(file);
				fileStates[f].dependencies.push_back(file);
			}

		}
	}

	for (auto const& file : filesOfInterest) {
		for (auto const& e : _projects) {
			auto& project = e.second;
			if (&project == this) continue;

			for (auto const& h : project.getAllHFilesFlat()) {
				if (file == h) {
					auto package = project.getPackagePath();
					auto pList   = utils::explode(package, "/");
					auto d       = pList.back() + "/" + project.getName();

					if (std::find(dep.begin(), dep.end(), d) == dep.end()) {
						dep.emplace_back(d);
					}
					break;
				}
			}
		}
	}

	return dep;
}

auto Project::getDefaultOptionalDependencies(Workspace* _workspace, std::map<std::string, Project> const& _projects) const -> Dependencies {
	auto& fileStates = _workspace->accessConfigFile().accessAutoFileStates();
	Dependencies dep;
	auto allFiles = getAllFiles({".cpp", ".c", ".h", ".hpp"});
	std::set<std::string> filesOfInterest;
	for (auto const& f : allFiles) {
		auto currentTime = getFileStates().getFileModTime(f);
		auto lastTime    = fileStates[f].lastChange;
		if (lastTime == currentTime and not fileStates[f].hasChanged) {
			for (auto const& file : fileStates[f].optDependencies) {
				filesOfInterest.insert(file);
			}
			continue;
		}
		fileStates[f].lastChange = currentTime;
		fileStates[f].hasChanged = true;

		std::ifstream ifs(f);
		std::string line;
		bool optionalSection = false;

		while (std::getline(ifs, line)) {
			auto parts = utils::explode(line, std::vector<std::string>{" ", "\t"});
			if (parts.size() == 1 && parts[0] == "#endif") {
				optionalSection = false;
			} else if (parts.size() == 2 && parts[0] == "#ifdef" && parts[1].substr(0, 7) == "BUSY_") {
				optionalSection = true;
			} else if (optionalSection
			    && parts.size() == 2 && parts[0] == "#include"
			    && parts[1].front() == '<' && parts[1].back() == '>') {

				auto pos1 = parts[1].find("<")+1;
				auto pos2 = parts[1].find(">")-pos1;

				auto file = parts[1].substr(pos1, pos2);
				filesOfInterest.insert(file);
				fileStates[f].optDependencies.push_back(file);
			}

		}
	}

	for (auto const& file : filesOfInterest) {
		for (auto const& e : _projects) {
			auto& project = e.second;
			if (&project == this) continue;

			for (auto const& h : project.getAllHFilesFlat()) {
				if (file == h) {
					auto package = project.getPackagePath();
					auto pList   = utils::explode(package, "/");
					auto d       = pList.back() + "/" + project.getName();
					if (std::find(dep.begin(), dep.end(), d) == dep.end()) {
						dep.emplace_back(d);
					}
					break;
				}
			}
		}
	}

	return dep;
}


auto Project::getAllFiles(std::set<std::string> const& _ending) const -> std::vector<std::string> {
	std::vector<std::string> files;

	std::vector<std::string> allPaths;
	allPaths.emplace_back(std::string("./") + getPackagePath() + "/src/" + getPath() + "/");
	for (auto const& i : getLegacy().includes) {
		allPaths.emplace_back(std::string("./") + getPackagePath() + "/" + i + "/");
	}

	for (auto const& p : allPaths) {
		if (not utils::dirExists(p)) continue;

		auto allFiles = utils::listFiles(p, true);
		for (auto const& f : allFiles) {
			for (auto const& ending : _ending) {
				if (utils::isEndingWith(f, ending)) {
					files.push_back(p + f);
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
	retList.push_back("-DABUILD");

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

