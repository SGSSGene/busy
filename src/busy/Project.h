#pragma once

#include <busyConfig/busyConfig.h>

namespace busy {
	class Package;

	class Project {
	public:
		enum class Type {
			Executable,
			StaticLibrary,
			SharedLibrary,
			Plugin
		};
	private:
		Package* mPackage;

		std::string mName;
		std::string mPath;

		Type        mType {Type::StaticLibrary};

		bool        mHasConfigEntry {false};
		bool        mWholeArchive {false};
		bool        mAutoDependenciesDiscovery {true};
		bool        mIsHeaderOnly {false};
		bool        mSingleFileProjects {false}; // Indicates that each .c/.cpp file should be treated as a project/library
		bool        mWarningsAsErrors {true};

		std::string mSourcePath;
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
		Project(busyConfig::Project const& _project, Package* _package, std::string const& folder);

		// Constructed from folder
		Project(std::string const& _name, Package* _package, std::string const& folder);


		auto getName() const -> std::string const& { return mName; }
		auto getFullName() const -> std::string;
		auto getFullName(std::string const& _inter) const -> std::string;
		auto getPath() const -> std::string const& { return mPath; }
		bool getHasConfigEntry() const { return mHasConfigEntry; }
		auto getType() const -> Type { return mType; }
		bool getIsUnitTest() const;
		bool getIsExample() const;
		bool getWholeArchive() const { return mWholeArchive; }
		bool getAutoDependenciesDiscovery() const { return mAutoDependenciesDiscovery; }
		auto getSourcePath() const -> std::string const& { return mSourcePath; }
		auto getIncludePaths() const -> std::vector<std::string> const& { return mIncludePaths; }
		auto getSystemIncludePaths() const -> std::vector<std::string> const& { return mSystemIncludePaths; }
		auto getDependencies() const -> std::vector<Project const*> const& { return mDependencies; }
		auto getDependenciesOnlyStatic(std::set<Project const*> const& _ignoreProject = {}) const -> std::vector<Project const*>;
		auto getDependenciesOnlyShared(std::set<Project const*> const& _ignoreProject = {}) const -> std::vector<Project const*>;
		auto getDependenciesRecursive(std::set<Project const*> const& _ignoreProject = {}) const -> std::vector<Project const*>;
		auto getDependenciesRecursiveOnlyStatic(std::set<Project const*> const& _ignoreProject = {}) const -> std::vector<Project const*>;
		auto getDependenciesRecursiveOnlyShared(std::set<Project const*> const& _ignoreProject = {}) const -> std::vector<Project const*>;
		auto getDependenciesRecursiveOnlyStaticNotOverShared(std::set<Project const*> const& _ignoreProject = {}) const -> std::vector<Project const*>;
		auto getDependenciesRecursiveOnlyStaticOverShared(std::set<Project const*> const& _ignoreProject = {}) const -> std::vector<Project const*>;
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
		bool getWarningsAsErrors() const { return mWarningsAsErrors; }

		auto getIncludeAndDependendPaths() const -> std::vector<std::string>;
		auto getSystemIncludeAndDependendPaths() const -> std::vector<std::string>;
		auto getLegacySystemIncludeAndDependendPaths() const -> std::vector<std::string>;

		void setType(Type _type) { mType = _type; }

	private:
		void discoverSourceFiles();
	public:
		void discoverDependencies();
	private:
		void discoverDependenciesInFile(std::string const& _file);

	};
	inline auto to_string(Project::Type type) -> std::string {
		switch (type) {
		case Project::Type::Executable:    return "executable";
		case Project::Type::StaticLibrary: return "staticLibrary";
		case Project::Type::SharedLibrary: return "sharedLibrary";
		case Project::Type::Plugin:        return "plugin";
		}
		throw std::runtime_error("unknown project type: " + std::to_string(int(type)));
	}

}
