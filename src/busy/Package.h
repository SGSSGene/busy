#pragma once

#include "Installation.h"
#include "Override.h"
#include "PackageURL.h"
#include "Project.h"
#include "Toolchain.h"
#include "Flavor.h"

#include <busyConfig/busyConfig.h>

namespace busy {
	class NeoWorkspace;

	using ExtRepositories = std::vector<PackageURL>;
	using Projects        = std::vector<Project>;
	using Installations   = std::vector<Installation>;
	using Toolchains      = std::vector<Toolchain>;
	using Overrides       = std::vector<Override>;
	using Flavors         = std::map<std::string, Flavor>;

	class Package {
	private:
		std::string     name;
		std::string     path;
		PackageURL      url;
		ExtRepositories extRepositories;
		Projects        projects;
		Overrides       overrides;
		Installations   installations;
		Toolchains      toolchains;
		Flavors         flavors;

		NeoWorkspace*   mWorkspace {nullptr};

	public:
		Package(PackageURL const& _url, busyConfig::Package const& _package);
		Package(PackageURL const& _url);

		void setWorkspace(NeoWorkspace* _workspace);



		auto getName() const -> std::string const&;
		void setName(std::string const& _name);

		void setPath(std::string const& _path);
		auto getPath() const -> std::string const&;

		auto getURL() const -> PackageURL const&;

		auto getExtRepositories() const -> ExtRepositories const&;
		auto getAllProjects() const -> Projects;
		auto getProjects() const -> Projects const&;
		auto accessProjects() -> Projects&;

		auto getOverrides() const -> Overrides const&;
		auto getInstallations() const -> Installations const&;
		auto getToolchains() const -> Toolchains const&;
		auto getFlavors() const -> Flavors const&;
	};

	auto readPackage(std::string const& _path, PackageURL _url = PackageURL()) -> Package;

}
