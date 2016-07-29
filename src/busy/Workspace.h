#pragma once

#include "FileStat.h"
#include "Package.h"
#include "Visitor.h"
#include <list>
#include <string>
#include <vector>

namespace busy {

class Workspace final {
public:
	Workspace();
	~Workspace();

	auto getPackageFolders() const -> std::vector<std::string> const&;
	auto getPackages() const -> std::list<Package> const&;

	bool hasPackage(std::string const& _name) const;
	auto getPackage(std::string const& _name) const -> Package const&;
	auto getPackage(std::string const& _name) -> Package&;
	auto getProject(std::string const& _name) const -> Project const&;

	auto getProjectAndDependencies(std::string const& _name = "") const -> std::vector<Project const*>;

	auto getFlavors() const -> std::map<std::string, Flavor const*>;
	auto getToolchains() const -> std::map<std::string, Toolchain const*>;

	auto getSelectedToolchain() const -> std::string;
	auto getSelectedBuildMode() const -> std::string;

	auto getFileStat(std::string const& _file) -> FileStat&;
private:
	void loadPackageFolders();
	void loadPackages();

	void discoverSystemToolchains();

private:
	std::vector<std::string> mPackageFolders;
	std::list<Package>    mPackages;

	std::map<std::string, Toolchain> mSystemToolchains;

	std::map<std::string, FileStat> mFileStats;

};

}
