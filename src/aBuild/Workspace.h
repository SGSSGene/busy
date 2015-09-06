#pragma once

#include "Package.h"
#include <jsonSerializer/jsonSerializer.h>

namespace aBuild {


class Workspace {
public:
	class ConfigFile {
	private:
		std::string flavor    { "release" };
		std::string toolchain { "" };

	public:
		auto getFlavor() const -> std::string const&     { return flavor; }
		void setFlavor(std::string const& _flavor)       { flavor = _flavor; }
		auto getToolchain() const -> std::string const&  { return toolchain; }
		void setToolchain(std::string const& _toolchain) { toolchain = _toolchain; }

		void serialize(jsonSerializer::Node& node) {
			node["flavor"]   % flavor;
			node["toochain"] % toolchain;
		}
	};
private:
	std::string path;
	ConfigFile  configFile;
public:

	Workspace(std::string const& _path);
	~Workspace();

	ConfigFile& accessConfigFile() { return configFile; }

	auto getAllMissingPackages()     const -> std::vector<PackageURL>;
	auto getAllValidPackages(bool _includingRoot = false) const -> std::vector<Package>;
	auto getAllInvalidPackages()     const -> std::vector<std::string>;
	auto getAllRequiredPackages()    const -> std::vector<PackageURL>;
	auto getAllNotRequiredPackages() const -> std::vector<std::string>;

	auto getAllRequiredProjects()    const -> std::map<std::string, Project>;

private:
	void createPackageFolder();
};

}
