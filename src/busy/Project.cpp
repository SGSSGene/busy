#include "Project.h"
#include "Package.h"
#include "Workspace.h"

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
	Project::Project(busyConfig::Project const& _project, Package* _package)
		: mPackage { _package }
	{
		mPath           = mPackage->getPath() + "/src";
		mName           = _project.name;
		mHasConfigEntry = true;
		mType           = _project.type;
		mWholeArchive   = _project.wholeArchive;
		mAutoDependenciesDiscovery = _project.mAutoDependenciesDiscovery;
		mSystemLibraries = _project.depLibraries;
		mSingleFileProjects = _project.mSingleFileProjects;

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
		for (auto const& f : _project.legacy.systemLibraries) {
			mSystemLibrariesPaths.push_back(f);
		}
		for (auto const& f : _project.legacy.linkingOption) {
			mLinkingOptions.push_back(f);
		}
		discoverSourceFiles();
		
		mIsHeaderOnly = (getCppFiles().size() == 0 && getCFiles().size() == 0);
	}

	Project::Project(std::string const& _name, Package* _package)
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
		mIsHeaderOnly = (getCppFiles().size() == 0 && getCFiles().size() == 0);
	}

	auto Project::getFullName() const -> std::string {
		return mPackage->getName() + "/" + getName();
	}
	bool Project::getIsUnitTest() const {
		return utils::isStartingWith(mName, "test");
	}
	bool Project::getIsExample() const {
		return utils::isStartingWith(mName, "example") or utils::isStartingWith(mName, "demo");
	}
	auto Project::getCppAndCFiles() const -> std::vector<std::string> {
		auto cppFiles = getCppFiles();
		for (auto const& f : getCFiles()) {
			cppFiles.push_back(f);
		}
		return cppFiles;
	}



	auto Project::getDependenciesRecursive(std::set<Project const*> const& _ignoreProject) const -> std::vector<Project const*> {
		std::vector<Project const*> retList;
		for (auto dep : getDependencies()) {
			if (_ignoreProject.count(dep) == 0) {
				retList.push_back(dep);
			}
		}
		auto iterateList = retList;
		for (auto const& project : iterateList) {
			for (auto p : project->getDependenciesRecursive(_ignoreProject)) {
				retList.push_back(p);
			}
		}
		std::map<Project const*, int> entryCount;
		for (auto entry : retList) {
			entryCount[entry] += 1;
		}
		retList.erase(std::remove_if(retList.begin(), retList.end(), [&entryCount] (Project const* p) {
			entryCount[p] -= 1;
			return entryCount[p] > 0;
		}), retList.end());

		return retList;
	}

	auto Project::getSystemLibrariesPathsRecursive() const -> std::vector<std::string> {
		auto retList = getSystemLibrariesPaths();
		for (auto const& project : getDependencies()) {
			for (auto p : project->getSystemLibrariesPathsRecursive()) {
				retList.push_back(p);
			}
		}

		std::map<std::string, int> entryCount;
		for (auto entry : retList) {
			entryCount[entry] += 1;
		}
		retList.erase(std::remove_if(retList.begin(), retList.end(), [&entryCount] (std::string p) {
			entryCount[p] -= 1;
			return entryCount[p] > 0;
		}), retList.end());

		return retList;
	}

	auto Project::getLinkingOptionsRecursive() const -> std::vector<std::string> {
		auto retList = getLinkingOptions();
		for (auto const& project : getDependencies()) {
			for (auto p : project->getLinkingOptionsRecursive()) {
				retList.push_back(p);
			}
		}

		std::map<std::string, int> entryCount;
		for (auto entry : retList) {
			entryCount[entry] += 1;
		}
		retList.erase(std::remove_if(retList.begin(), retList.end(), [&entryCount] (std::string p) {
			entryCount[p] -= 1;
			return entryCount[p] > 0;
		}), retList.end());

		return retList;
	}

	void Project::discoverSourceFiles() {
		mSourceFiles["cpp"]       = {};
		mSourceFiles["c"]         = {};
		mSourceFiles["incl"]      = {};
		mSourceFiles["incl-flat"] = {};
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
				mSourceFiles["incl"].push_back(dir + "/" + f);
				if (&dir == &includePaths[0]) {
					mSourceFiles["incl-flat"].push_back(getName() + "/" + f);
				} else {
					mSourceFiles["incl-flat"].push_back(f);
				}
			}
		}
	}

	auto Project::getIncludeAndDependendPaths() const -> std::vector<std::string> {
		auto includePaths = getIncludePaths();
		includePaths.push_back(includePaths.front() + "/" + getName());
		return includePaths;
	}
	auto Project::getSystemIncludeAndDependendPaths() const -> std::vector<std::string> {
		auto includePaths = getSystemIncludePaths();

		std::set<std::string> alreadyAdded;

		for (auto dep : getDependenciesRecursive()) {
			for (auto& p : dep->getSystemIncludePaths()) {
				if (alreadyAdded.count(p) == 0) {
					alreadyAdded.insert(p);
					includePaths.emplace_back(std::move(p));
				}
			}
			for (auto& p : dep->getIncludePaths()) {
				if (alreadyAdded.count(p) == 0) {
					alreadyAdded.insert(p);
					includePaths.emplace_back(std::move(p));
				}
			}
		}
		return includePaths;
	}

	void Project::discoverDependencies() {

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
	void Project::discoverDependenciesInFile(std::string const& _file) {
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
			auto packages = mPackage->getAllDependendPackages();
			packages.push_back(mPackage);
			for (auto const& file : includesOutsideOfThisProject) {
				bool found = false;
				for (auto const& package : packages) {
					for (Project const& project : package->getProjects()) {
						auto fileToCheck = file;
						for (auto const& include : project.getIncludeFilesFlat()) {
							if (fileToCheck == include) {
								if (std::find(mDependencies.begin(), mDependencies.end(), &project) == mDependencies.end()) {
									if (getFullName() != project.getFullName()) {
										dependenciesAsString.insert(project.getFullName());
									}
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
					for (Project const& project : package.getProjects()) {
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
