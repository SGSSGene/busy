#pragma once

#include "Package.h"
#include <serializer/serializer.h>

namespace aBuild {


class Workspace {
public:
	class ConfigFile {
	private:
		std::string buildMode { "release" };
		std::string toolchain { "" };
		uint64_t    lastCompileTime;

	public:
		auto getBuildMode() const -> std::string const&  { return buildMode; }
		void setBuildMode(std::string const& _buildMode) { buildMode = _buildMode; }
		auto getToolchain() const -> std::string const&  { return toolchain; }
		void setToolchain(std::string const& _toolchain) { toolchain = _toolchain; }
		auto getLastCompileTime() const -> int64_t       { return lastCompileTime; }
		void setLastCompileTime(int64_t _time)           { lastCompileTime = _time; }

		template<typename Node>
		void serialize(Node& node) {
			node["buildMode"]       % buildMode;
			node["toochain"]        % toolchain;
			node["lastCompileTime"] % lastCompileTime or 0ull;
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
