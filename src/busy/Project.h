#pragma once

#include <busyConfig/busyConfig.h>

namespace busy {
	class Package;

	class Project {
	private:
		Package* mPackage;

		std::string mName;
		std::string mPath;
		std::string mType {"library"};

		bool        mHasConfigEntry {false};
		bool        mWholeArchive {false};
		bool        mAutoDependenciesDiscovery {true};
		bool        mIsHeaderOnly {false};
		bool        mSingleFileProjects {false}; // Indicates that each .c/.cpp file should be treated as a project/library

		std::vector<std::string> mSourcePaths;
		std::vector<std::string> mIncludePaths;
		std::vector<std::string> mSystemIncludePaths;
		std::set<std::string>    mDependenciesAsString;
		std::vector<Project const*> mDependencies;
		std::map<std::string, std::vector<std::string>> mSourceFiles;
		std::vector<std::string> mSystemLibraries;
		std::vector<std::string> mSystemLibrariesPaths;
		std::vector<std::string> mLinkingOptions;
	public:
		// Constructed from config file
		Project(busyConfig::Project const& _project, Package* _package);

		// Constructed from folder
		Project(std::string const& _name, Package* _package);

		auto getName() const -> std::string const& { return mName; }
		auto getFullName() const -> std::string;
		auto getPath() const -> std::string const& { return mPath; }
		bool getHasConfigEntry() const { return mHasConfigEntry; }
		auto getType() const -> std::string const& { return mType; }
		bool getIsUnitTest() const;
		bool getIsExample() const;
		bool getWholeArchive() const { return mWholeArchive; }
		bool getAutoDependenciesDiscovery() const { return mAutoDependenciesDiscovery; }
		auto getSourcePaths() const -> std::vector<std::string> const& { return mSourcePaths; }
		auto getIncludePaths() const -> std::vector<std::string> const& { return mIncludePaths; }
		auto getSystemIncludePaths() const -> std::vector<std::string> const& { return mSystemIncludePaths; }
		auto getDependencies() const -> std::vector<Project const*> const& { return mDependencies; }
		auto getDependenciesRecursive(std::set<Project const*> const& _ignoreProject = {}) const -> std::vector<Project const*>;
		auto getCFiles() const -> std::vector<std::string> const& { return mSourceFiles.at("c"); }
		auto getCppFiles() const -> std::vector<std::string> const& { return mSourceFiles.at("cpp"); }
		auto getCppAndCFiles() const -> std::vector<std::string>;
		auto getIncludeFiles() const -> std::vector<std::string> const& { return mSourceFiles.at("incl"); }
		auto getIncludeFilesFlat() const -> std::vector<std::string> const& { return mSourceFiles.at("incl-flat"); }
		auto getSystemLibraries() const -> std::vector<std::string> const& { return mSystemLibraries; }
		auto getSystemLibrariesPaths() const -> std::vector<std::string> const& { return mSystemLibrariesPaths; }
		auto getLinkingOptions() const -> std::vector<std::string> const& { return mLinkingOptions; }
		auto getSystemLibrariesPathsRecursive() const -> std::vector<std::string>;
		auto getLinkingOptionsRecursive() const -> std::vector<std::string>;

		bool getIsHeaderOnly() const { return mIsHeaderOnly; }
		bool getIsSingleFileProjects() const { return mSingleFileProjects; }

		auto getIncludeAndDependendPaths() const -> std::vector<std::string>;
		auto getSystemIncludeAndDependendPaths() const -> std::vector<std::string>;
		auto getLegacySystemIncludeAndDependendPaths() const -> std::vector<std::string>;

	private:
		void discoverSourceFiles();
	public:
		void discoverDependencies();
	private:
		void discoverDependenciesInFile(std::string const& _file);

	};
}
