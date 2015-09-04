#pragma once

#include "Package.h"
#include <jsonSerializer/jsonSerializer.h>

namespace aBuild {


class Workspace {
public:
	class ConfigFile {
	private:
		std::string activeFlavor { "release" };

	public:
		auto getActiveFlavor() const -> std::string const& { return activeFlavor; }
		void setActiveFlavor(std::string const& _flavor) { activeFlavor = _flavor; }
		void serialize(jsonSerializer::Node& node) {
			node["activeFlavor"] % activeFlavor;
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
	auto getAllValidPackages()       const -> std::vector<Package>;
	auto getAllInvalidPackages()     const -> std::vector<std::string>;
	auto getAllRequiredPackages()    const -> std::vector<PackageURL>;
	auto getAllNotRequiredPackages() const -> std::vector<std::string>;

	auto getAllRequiredProjects()    const -> std::map<std::string, Project>;

private:
	void createPackageFolder();
};

}
