#pragma once

#include "NeoFileStat.h"
#include "NeoPackage.h"
#include <list>
#include <string>
#include <vector>

namespace busy {

class NeoWorkspace final {
public:
	NeoWorkspace();
	~NeoWorkspace();

	auto getPackageFolders() const -> std::vector<std::string> const&;
	auto getPackages() const -> std::list<NeoPackage> const&;

	bool hasPackage(std::string const& _name) const;
	auto getPackage(std::string const& _name) const -> NeoPackage const&;
	auto getPackage(std::string const& _name) -> NeoPackage&;
	auto getProject(std::string const& _name) const -> NeoProject const&;

	auto getProjectAndDependencies(std::string const& _name = "") const -> std::vector<NeoProject const*>;

	auto getFlavors() const -> std::map<std::string, NeoFlavor const*>;
	auto getToolchains() const -> std::map<std::string, NeoToolchain const*>;

	auto getFileStat(std::string const& _file) -> NeoFileStat&;
private:
	void loadPackageFolders();
	void loadPackages();

	void discoverSystemToolchains();

private:
	std::vector<std::string> mPackageFolders;
	std::list<NeoPackage>    mPackages;

	std::map<std::string, NeoToolchain> mSystemToolchains;

	std::map<std::string, NeoFileStat> mNeoFileStats;

};

}
