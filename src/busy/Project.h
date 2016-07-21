#pragma once


#include <serializer/serializer.h>
#include <busyUtils/busyUtils.h>

namespace busy {
	class Workspace;

	using Dependencies = std::vector<std::string>;
	using DepLibraries = std::vector<std::string>;
	using Defines      = std::vector<std::string>;

	struct ProjectLegacy {
		std::vector<std::string> includes;
		std::vector<std::string> systemIncludes;
		std::vector<std::string> systemLibraries;
		std::vector<std::string> linkingOption;

		template<typename Node>
		void serialize(Node& node) {
			node["includes"]        % includes;
			node["systemIncludes"]  % systemIncludes;
			node["systemLibraries"] % systemLibraries;
			node["linkingOption"]   % linkingOption;
		}
	};


	class Project {
	private:
		std::string  path;
		std::string  packagePath;
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

		mutable std::vector<std::string> cppFiles;
		mutable std::vector<std::string> cFiles;
		mutable std::vector<std::string> hFiles;
		mutable std::vector<std::string> hFilesFlat;

		bool mNeedsSharedVersion { false };

		ProjectLegacy legacy;

	public:
		Project()
			: packagePath  { "." }
			, noWarnings   { false }
			, wholeArchive { false }
			, mAutoDependenciesDiscovery { true  }
			, mIgnore      { false }
		{}

		template<typename Node>
		void serialize(Node& node) {
			node["name"]                 % path;
			node["dependencies"]         % dependencies;
			node["optionalDependencies"] % optionalDependencies;
			node["type"]                 % type                 or getDefaultTypeByName();
			node["legacy"]               % legacy;
			node["depLibraries"]         % depLibraries;
			node["defines"]              % defines;
			node["noWarnings"]           % noWarnings           or bool(false);
			node["wholeArchive"]         % wholeArchive         or bool(false);
			node["autoDependenciesDiscovery"] % mAutoDependenciesDiscovery or bool(true);
			node["ignore"]               % mIgnore              or bool(false);
			node["linkAsShared"]         % linkAsShared;
		}
		void set(std::string const& _name) {
			path = _name;
			type = getDefaultTypeByName();
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
		auto getPath() const -> std::string const& {
			return path;
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
