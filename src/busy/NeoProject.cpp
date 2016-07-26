#include "NeoProject.h"
#include "NeoPackage.h"
#include "NeoWorkspace.h"

#include <algorithm>
#include <busyUtils/busyUtils.h>
#include <cstring>
#include <fstream>
#include <iostream>

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


namespace busy {
	NeoProject::NeoProject(busyConfig::Project const& _project, NeoPackage* _package)
		: mPackage { _package }
	{
		mPath           = mPackage->getPath() + "/src";
		mName           = _project.name;
		mHasConfigEntry = true;
		mType           = _project.type;
		mWholeArchive   = _project.wholeArchive;
		mAutoDependenciesDiscovery = _project.mAutoDependenciesDiscovery;

		for (auto s : _project.dependencies) {
			mDependenciesAsString.insert(s);
		}

		mSourcePaths.emplace_back(mPackage->getPath() + "/src");
		mIncludePaths.emplace_back(mPackage->getPath() + "/src");
		for (auto const& f : _project.legacy.includes) {
			mIncludePaths.emplace_back(mPackage->getPath() + "/" + f);
		}
		for (auto const& f : _project.legacy.systemIncludes) {
			mSystemIncludePaths.emplace_back(f);
		}
		discoverSourceFiles();
	}

	NeoProject::NeoProject(std::string const& _name, NeoPackage* _package)
		: mPackage { _package }
	{
		mPath = mPackage->getPath() + "/src";
		mName = _name;
		if (utils::isStartingWith(mName, "test")) {
			mType = "executable";
		}

		mSourcePaths.emplace_back(mPackage->getPath() + "/src");
		mIncludePaths.emplace_back(mPackage->getPath() + "/src");

		discoverSourceFiles();
	}

	auto NeoProject::getFullName() const -> std::string {
		return mPackage->getName() + "/" + getName();
	}

	void NeoProject::discoverSourceFiles() {
		mSourceFiles["cpp"] = {};
		mSourceFiles["c"]   = {};
		mSourceFiles["incl"]   = {};
		// Discover cpp and c files
		auto sourcePaths = getSourcePaths();
		sourcePaths[0] += "/" + getName();
		for (auto const& dir : sourcePaths) {
			for (auto const &f : utils::listFiles(dir, true)) {
				if (utils::isEndingWith(f, ".cpp")) {
					mSourceFiles["cpp"].push_back(dir + "/" + f);
				} else if (utils::isEndingWith(f, ".c")) {
					mSourceFiles["c"].push_back(dir + "/" + f);
				}
			}
		}
		// Discover header files
		auto includePaths = getIncludePaths();
		includePaths[0] += "/" + getName();
		for (auto const& dir : includePaths) {
			for (auto const& f : utils::listFiles(dir, true)) {
				if (utils::isEndingWith(f, ".h") or utils::isEndingWith(f, ".hpp")) {
					mSourceFiles["incl"].push_back(dir + "/" + f);
				}
			}
		}
	}

	void NeoProject::discoverDependencies() {

		// scan all files to detect dependencies
		if (mAutoDependenciesDiscovery) {
			for (auto s : {"cpp", "c", "incl"}) {
				for (auto const& file : mSourceFiles.at(s)) {
					discoverDependenciesInFile(file);
				}
			}
		}
		for (auto const& d : mDependenciesAsString) {
			mDependencies.push_back(&mPackage->getWorkspace()->getProject(d));
		}
	}
	void NeoProject::discoverDependenciesInFile(std::string const& _file) {
		// First check if this file is cached
		auto& fileStat = mPackage->getWorkspace()->getFileStat(_file);


		// If file modification time has changed, rescan it
		auto modTime = utils::getFileModificationTime(_file);
		if (modTime != fileStat.mFileDiscovery.lastChange) {
			std::ifstream ifs(_file);
			std::string line;

			std::set<std::string> dependenciesAsString;

			std::set<std::string> includesOutsideOfThisProject;
			std::set<std::string> includesOutsideOfThisProjectOptional;

			bool optionalSection = false;

			while (std::getline(ifs, line)) {
				// check if it is '#endif'
				if (checkIfMakroEndif(line.c_str())) {
					optionalSection = false;
				} else if (checkIfMakroIfAndBusy(line.c_str())) {
					optionalSection = true;
				} else if (checkIfMakroSystemInclude(line.c_str())) {
					auto parts = utils::explode(line, std::vector<std::string>{" ", "\t"});

					std::string includeFile;
					if (parts.size() == 0) continue;
					if (parts.size() == 1) {
						includeFile = parts[0];
					} else {
						includeFile = parts[1];
					}

					auto pos1 = includeFile.find("<")+1;
					auto pos2 = includeFile.find(">")-pos1;

					auto file = includeFile.substr(pos1, pos2);

					if (optionalSection) {
						includesOutsideOfThisProjectOptional.insert(file);
						includesOutsideOfThisProject.erase(file);
					} else if (includesOutsideOfThisProjectOptional.count(file) == 0) {
						includesOutsideOfThisProject.insert(file);
					}
				}
			}

			// Check all found #include<...> statements, and check if these refer to a known project
			for (auto const& file : includesOutsideOfThisProject) {
				bool found = false;
				for (auto const& package : mPackage->getExternalPackages()) {
					for (NeoProject const& project : package->getProjects()) {
						auto fileToCheck = project.getPath() + "/" + file;
						for (auto const& include : project.getIncludeFiles()) {
							if (fileToCheck == include) {
								if (std::find(mDependencies.begin(), mDependencies.end(), &project) == mDependencies.end()) {
									dependenciesAsString.insert(project.getFullName());
								}
								found = true;
								break;
							}
						}
						if (found) break;
					}
					if (found) break;
				}
			}

			// Do the same for optional dependencies, these must search through all packages
			// Check all found #include<...> statements, and check if these refer to a known project
			for (auto const& file : includesOutsideOfThisProjectOptional) {
				bool found = false;
				for (auto const& package : mPackage->getWorkspace()->getPackages()) {
					for (NeoProject const& project : package.getProjects()) {
						auto fileToCheck = project.getPath() + "/" + file;
						for (auto const& include : project.getIncludeFiles()) {
							if (fileToCheck == include) {
								if (std::find(mDependencies.begin(), mDependencies.end(), &project) == mDependencies.end()) {
									dependenciesAsString.insert(project.getFullName());
								}
								found = true;
								break;
							}
						}
						if (found) break;
					}
					if (found) break;
				}
			}
			fileStat.mFileDiscovery.lastChange = modTime;
			fileStat.mFileDiscovery.dependenciesAsString = dependenciesAsString;
		}
		for (auto const& s : fileStat.mFileDiscovery.dependenciesAsString) {
			mDependenciesAsString.insert(s);
		}
	}
}
