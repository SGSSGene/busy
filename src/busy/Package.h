#pragma once

#include "Installation.h"
#include "Override.h"
#include "PackageURL.h"
#include "Project.h"
#include "Toolchain.h"
#include "Flavor.h"

namespace busy {

	using ExtRepositories = std::vector<PackageURL>;
	using Projects        = std::vector<Project>;
	using Installations   = std::vector<Installation>;
	using Toolchains      = std::vector<Toolchain>;
	using Overrides       = std::vector<Override>;
	using Flavors         = std::map<std::string, Flavor>;

	class Package {
	private:
		std::string     name;
		PackageURL      url;
		ExtRepositories extRepositories;
		Projects        projects;
		Overrides       overrides;
		Installations   installations;
		Toolchains      toolchains;
		Flavors         flavors;

	public:
		Package(PackageURL const& _url);

		auto getName() const -> std::string const&;
		void setName(std::string const& _name);

		auto getURL() const -> PackageURL const&;

		auto getExtRepositories() const -> ExtRepositories const&;
		auto getProjects() const -> Projects const&;
		auto accessProjects() -> Projects&;

		auto getOverrides() const -> Overrides const&;
		auto getInstallations() const -> Installations const&;
		auto getToolchains() const -> Toolchains const&;
		auto getFlavors() const -> Flavors const&;

		template <typename Node>
		void serialize(Node& node) {
			node["name"]            % name;
			node["extRepositories"] % extRepositories;
			node["projects"]        % projects;
			node["overrides"]       % overrides;
			node["installations"]   % installations;
			node["toolchains"]      % toolchains;
			node["flavors"]         % flavors;
			for (auto& p : projects) {
				p.setPackagePath(url.getPath());
			}
		}

	};
}
