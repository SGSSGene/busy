#pragma once

#include <busyConfig/busyConfig.h>
#include <busyUtils/busyUtils.h>

namespace busy {
	class Workspace;
	class Package;
	class NeoWorkspace;

	using Dependencies = std::vector<std::string>;
	using DepLibraries = std::vector<std::string>;
	using Defines      = std::vector<std::string>;

	struct ProjectLegacy {
		ProjectLegacy() {}
		ProjectLegacy(busyConfig::ProjectLegacy _legacy) {
			includes        = _legacy.includes;
			systemIncludes  = _legacy.systemIncludes;
			systemLibraries = _legacy.systemLibraries;
			linkingOption   = _legacy.linkingOption;
		}
		std::vector<std::string> includes;
		std::vector<std::string> systemIncludes;
		std::vector<std::string> systemLibraries;
		std::vector<std::string> linkingOption;
	};


	class Project {
	private:
		std::string  path;
		std::string  packagePath;
		std::string  mFullName;
		bool         mHasConfigEntry { true };
		Dependencies dependencies;
		Dependencies optionalDependencies;
		DepLibraries depLibraries;
		Defines      defines;
		bool         noWarnings;
		bool         wholeArchive;
		bool         mAutoDependenciesDiscovery;
		bool         mIgnore;
		std::string  type;
		std::vector<std::string> linkAsShared;
		NeoWorkspace* mWorkspace {nullptr};


		mutable std::vector<std::string> cppFiles;
		mutable std::vector<std::string> cFiles;
		mutable std::vector<std::string> hFiles;
		mutable std::vector<std::string> hFilesFlat;

		mutable bool mCachedDependenciesValid {false};
		mutable Dependencies mCachedDefaultDependencies;
		mutable Dependencies mCachedOptionalDependencies;

		bool mNeedsSharedVersion { false };

		ProjectLegacy legacy;

	public:
		Project(busyConfig::Project const& _project);
		Project()
			: packagePath  { "." }
			, noWarnings   { false }
			, wholeArchive { false }
			, mAutoDependenciesDiscovery { true  }
			, mIgnore      { false }
		{}

		void setWorkspace(NeoWorkspace* _workspace);

		void set(std::string const& _name) {
			path = _name;
			type = getDefaultTypeByName();
		}
		void setHasConfigEntry(bool _hasEntry) {
			mHasConfigEntry = _hasEntry;
		}
		bool getHasConfigEntry() const {
			return mHasConfigEntry;
		}

		void setNeedsSharedVersion() {
			mNeedsSharedVersion = true;
		}
		bool getNeedsSharedVersion() const {
			return mNeedsSharedVersion;
		}

		auto getType() const -> std::string const& {
			return type;
		}

		auto getLegacy() const -> ProjectLegacy const& {
			return legacy;
		}
		auto getName() const -> std::string {
			auto l = utils::explode(getPath(), "/");
			if (l.size() == 0) throw std::runtime_error("project name is invalid: " + getPath());
			return l[l.size()-1];
		}
		void setFullName(std::string const& _fullName) {
			mFullName = _fullName;
		}
		auto getFullName() const -> std::string const& {
			return mFullName;
		}
		auto getSourcePaths() const -> std::vector<std::string>;
		auto getIncludePaths() const -> std::vector<std::string>;

		auto getPath() const -> std::string {
			return packagePath + "/src/" + path;
		}
		auto getPackagePath() const -> std::string const& {
			return packagePath;
		}
		void setPackagePath(std::string const& s) {
			packagePath = s;
		}
		void setDependencies(Dependencies _dep) {
			dependencies = std::move(_dep);
		}
		void setOptionalDependencies(Dependencies _dep) {
			optionalDependencies = std::move(_dep);
		}

		auto getDependencies() const -> Dependencies const& {
			return dependencies;
		}
		auto getOptionalDependencies() const -> Dependencies const& {
			return optionalDependencies;
		}
		auto getDepLibraries() const -> DepLibraries const& {
			return depLibraries;
		}
		auto getDefines() const -> Defines const& {
			return defines;
		}
		bool getNoWarnings() const {
			return noWarnings;
		}
		bool getWholeArchive() const {
			return wholeArchive;
		}
		auto getLinkAsShared() const -> std::vector<std::string> const& {
			return linkAsShared;
		}
		void setAutoDependenciesDiscovery(bool _auto) {
			mAutoDependenciesDiscovery = _auto;
		}
		bool getAutoDependenciesDiscovery() const {
			return mAutoDependenciesDiscovery;
		}
		bool getIgnore() const {
			return mIgnore;
		}

		void quickFix();

		auto getDefaultTypeByName() const -> std::string;


		void getDefaultAndOptionalDependencies(Workspace* _workspace, std::map<std::string, Project> const& _project) const;
		auto getDefaultDependencies(Workspace* _workspace, std::map<std::string, Project> const& _projects) const -> Dependencies;
		auto getDefaultOptionalDependencies(Workspace* _workspace, std::map<std::string, Project> const& _projects) const -> Dependencies;

		auto getAllFiles(std::set<std::string> const& _ending) const -> std::vector<std::string>;
		auto getAllFilesFlat(std::set<std::string> const& _ending, bool noending = false) const -> std::vector<std::string>;
		auto getAllFilesFlatNoEnding() const -> std::vector<std::string>;

		auto getAllCppFiles() -> std::vector<std::string>&;
		auto getAllCFiles() -> std::vector<std::string>&;
		auto getAllHFiles() const -> std::vector<std::string> const&;
		auto getAllHFilesFlat() const -> std::vector<std::string> const&;

		auto getComIncludePaths() const -> std::vector<std::string>;
		auto getComSystemIncludePaths(std::set<Project*> const& _dependencies) const -> std::vector<std::string>;
		auto getComDefines(std::set<Project*> const& _dependencies) const -> std::vector<std::string>;
	};
}
