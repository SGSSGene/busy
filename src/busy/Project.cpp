#include "Project.h"
#include "Package.h"
#include "Workspace.h"

#include "analyse/File.h"
#include "analyse/Project.h"

#include <algorithm>
#include <busyUtils/busyUtils.h>

namespace busy {
	Project::Project(busyConfig::Project const& _project, Package* _package, std::string const& folder)
		: mPackage { _package }
	{
		mPath           = folder;
		mName           = _project.name;
		mHasConfigEntry = true;
		if (_project.type == "executable") {
			mType = Type::Executable;
		} else if (_project.type == "library"
		           or _project.type == "staticLibrary") {
			mType = Type::StaticLibrary;
		} else if (_project.type == "plugin") {
			mType = Type::Plugin;
		} else {
			//!TODO list all possible types
			throw std::runtime_error("Unknown project type: " + _project.type + " must be of the type \"(executable, library, staticLibrary, sharedLibrary...\"");
		}
		mWholeArchive              = _project.wholeArchive;
		mAutoDependenciesDiscovery = _project.mAutoDependenciesDiscovery;
		mSystemLibraries           = _project.depLibraries;
		mSingleFileProjects        = _project.mSingleFileProjects;
		mWarningsAsErrors          = _project.warningsAsErrors;

		for (auto s : _project.dependencies) {
			mDependenciesAsString.insert(s);
		}

		mSourcePath = mPath + "/" + getName();
		mIncludePaths.emplace_back(mPath);
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

	Project::Project(std::string const& _name, Package* _package, std::string const& folder)
		: mPackage { _package }
	{
		mPath = folder;
		mName = _name;
		if (getIsUnitTest() or getIsExample()) {
			mType = Type::Executable;
		}

		mSourcePath = mPath + "/" + getName();
		mIncludePaths.emplace_back(mPath);

		discoverSourceFiles();
		mIsHeaderOnly = (getCppFiles().size() == 0 && getCFiles().size() == 0);
	}

	auto Project::getFullName() const -> std::string {
		return mPackage->getName() + "/" + getName();
	}
	auto Project::getFullName(std::string const& _inter) const -> std::string {
		return mPackage->getName() + "/" + _inter + getName();
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


	auto Project::getDependenciesOnlyStatic(std::set<Project const*> const& _ignoreProject) const -> std::vector<Project const*> {
		std::vector<Project const*> retList;
		for (auto dep : getDependencies()) {
			if (_ignoreProject.count(dep) > 0) continue;
			if (dep->getType() != Project::Type::StaticLibrary) continue;
			retList.push_back(dep);
		}
		return retList;
	}
	auto Project::getDependenciesOnlyShared(std::set<Project const*> const& _ignoreProject) const -> std::vector<Project const*> {
		std::vector<Project const*> retList;
		for (auto dep : getDependencies()) {
			if (_ignoreProject.count(dep) > 0) continue;
			if (dep->getType() != Project::Type::SharedLibrary) continue;
			retList.push_back(dep);
		}
		return retList;
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
	auto Project::getDependenciesRecursiveOnlyStatic(std::set<Project const*> const& _ignoreProject) const -> std::vector<Project const*> {
		std::vector<Project const*> retList = getDependenciesOnlyStatic(_ignoreProject);
		auto iterateList = retList;
		for (auto const& project : iterateList) {
			for (auto p : project->getDependenciesRecursiveOnlyStatic(_ignoreProject)) {
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
	auto Project::getDependenciesRecursiveOnlyShared(std::set<Project const*> const& _ignoreProject) const -> std::vector<Project const*> {
		std::vector<Project const*> retList = getDependenciesRecursive(_ignoreProject);

		retList.erase(std::remove_if(retList.begin(), retList.end(), [] (Project const* p) {
			return p->getType() != Project::Type::SharedLibrary;
		}), retList.end());

		return retList;
	}
	auto Project::getDependenciesRecursiveOnlyStaticNotOverShared(std::set<Project const*> const& _ignoreProject) const -> std::vector<Project const*> {
		auto retList = getDependenciesRecursiveOnlyStatic(_ignoreProject);
		auto shared  = getDependenciesRecursiveOnlyStaticOverShared(_ignoreProject);


		retList.erase(std::remove_if(retList.begin(), retList.end(), [&] (Project const* p) {
			for (auto const & s : shared) {
				if (s == p) return true;
			}
			return false;
		}), retList.end());
		return retList;
	}
	auto Project::getDependenciesRecursiveOnlyStaticOverShared(std::set<Project const*> const& _ignoreProject) const -> std::vector<Project const*> {
		std::vector<Project const*> retList;
		auto allShared = getDependenciesRecursiveOnlyShared(_ignoreProject);

		for (auto const& shared : allShared) {
			for (auto const & p : shared->getDependenciesRecursive(_ignoreProject)) {
				if (p->getType() == Project::Type::StaticLibrary) {
					retList.push_back(p);
				}
			}
		}
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
		auto includes = getIncludePaths();
		includes.erase(includes.begin());

		analyse::Project project(getName(), getSourcePath(), includes);
		
		mSourceFiles = project.getSourceFiles();
	}

	auto Project::getIncludeAndDependendPaths() const -> std::vector<std::string> {
		auto includePaths = getIncludePaths();
		includePaths.push_back(includePaths.front() + "/" + getName());
		return includePaths;
	}
	auto Project::getSystemIncludeAndDependendPaths() const -> std::vector<std::string> {
		std::vector<std::string> includePaths;

		std::set<std::string> alreadyAdded;

		for (auto dep : getDependenciesRecursive()) {
			for (auto& p : dep->getIncludePaths()) {
				if (alreadyAdded.count(p) == 0) {
					alreadyAdded.insert(p);
					includePaths.emplace_back(std::move(p));
				}
			}
		}
		return includePaths;
	}
	auto Project::getLegacySystemIncludeAndDependendPaths() const -> std::vector<std::string> {
		auto includePaths = getSystemIncludePaths();

		std::set<std::string> alreadyAdded;

		for (auto dep : getDependenciesRecursive()) {
			for (auto& p : dep->getSystemIncludePaths()) {
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
			if (mPackage->getWorkspace()->hasProject(d)) {
				mDependencies.push_back(&mPackage->getWorkspace()->getProject(d));
			}
		}
	}
	void Project::discoverDependenciesInFile(std::string const& _file) {
		// First check if this file is cached
		auto& fileStat = mPackage->getWorkspace()->getFileStat(_file);


		// If file modification time has changed, rescan it
		auto modTime = utils::getFileModificationTime(_file);
		if (modTime != fileStat.mFileDiscovery.lastChange) {
			analyse::File file(_file);

			auto includesOutsideOfThisProject         = file.getIncludes();
			auto includesOutsideOfThisProjectOptional = file.getIncludesOptional();

			std::set<std::string> dependenciesAsString;

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
