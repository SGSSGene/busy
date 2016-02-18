#pragma once

#include "Package.h"
#include <serializer/serializer.h>

namespace aBuild {


class Workspace {
public:
	struct FileState {
		int64_t lastChange {0};
		std::vector<std::string> dependencies;
		std::vector<std::string> optDependencies;

		template <typename Node>
		void serialize(Node& node) {
			node["lastChange"]      % lastChange;
			node["dependencies"]    % dependencies;
			node["optDependencies"] % optDependencies;
		}
	};
	class ConfigFile {
	private:
		std::string buildMode { "debug" };
		std::string toolchain { "" };
		uint64_t    lastCompileTime;

		std::map<std::string, FileState> mAutoFileStates;

	public:
		auto getBuildMode() const -> std::string const&  { return buildMode; }
		void setBuildMode(std::string const& _buildMode) { buildMode = _buildMode; }
		auto getToolchain() const -> std::string const&  { return toolchain; }
		void setToolchain(std::string const& _toolchain) { toolchain = _toolchain; }
		auto getLastCompileTime() const -> int64_t       { return lastCompileTime; }
		void setLastCompileTime(int64_t _time)           { lastCompileTime = _time; }

		auto accessAutoFileStates() -> std::map<std::string, FileState>& { return mAutoFileStates; }


		template<typename Node>
		void serialize(Node& node) {
			node["buildMode"]       % buildMode or std::string("debug");
			node["toolchain"]       % toolchain;
			node["lastCompileTime"] % lastCompileTime or 0ull;
			node["autoFileStates"]  % mAutoFileStates;
		}
	};
private:
	std::string path;
	ConfigFile  configFile;
public:

	Workspace(std::string const& _path);
	~Workspace();

	void save();

	ConfigFile& accessConfigFile() { return configFile; }

	auto getAllMissingPackages()     const -> std::vector<PackageURL>;
	auto getAllValidPackages(bool _includingRoot = false) const -> std::vector<Package>;
	auto getAllInvalidPackages()     const -> std::vector<std::string>;
	auto getAllRequiredPackages()    const -> std::vector<PackageURL>;
	auto getAllNotRequiredPackages() const -> std::vector<std::string>;

	auto getAllRequiredProjects()    const -> std::map<std::string, Project>;

	auto getExcludedProjects() const -> std::set<std::string>;

private:
	void createABuildFolder();
	void createPackageFolder();
};

}
