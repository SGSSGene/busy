#pragma once

#include "Package.h"
#include "Visitor.h"
#include "WorkspaceConfig.h"
#include <list>

namespace busy {

class Workspace final {
public:
	Workspace(bool _noSaving = false);
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
	auto getBuildModes() const -> std::vector<std::string>;

	auto getSelectedToolchain() const -> std::string;
	auto getSelectedBuildMode() const -> std::string;

	void setSelectedToolchain(std::string const& _toolchainName);
	void setSelectedBuildMode(std::string const& _buildMode);

	void setFlavor(std::string const& _flavor);

	auto getFileStat(std::string const& _file) -> FileStat&;

	auto getExcludedProjects(std::string const& _toolchain) const -> std::set<Project const*>;
private:
	void loadPackageFolders();
	void loadPackages();

	void discoverSystemToolchains();

private:
	std::vector<std::string> mPackageFolders;
	std::list<Package>       mPackages;

	std::map<std::string, Toolchain> mSystemToolchains;

	WorkspaceConfig mConfig;

	bool mNoSaving { false };
};

}
