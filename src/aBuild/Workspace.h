#pragma once

#include "Package.h"

namespace aBuild {

class Workspace {
private:
	std::string          path;

public:
	Workspace(std::string const& _path);

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
