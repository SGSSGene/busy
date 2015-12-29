#pragma once

#include "Installation.h"
#include "Override.h"
#include "PackageURL.h"
#include "Project.h"
#include "Toolchain.h"

namespace aBuild {

	using ExtDependencies = std::vector<PackageURL>;
	using Projects        = std::vector<Project>;
	using Installations   = std::vector<Installation>;
	using Toolchains      = std::vector<Toolchain>;
	using Overrides       = std::vector<Override>;

	class Package {
	private:
		std::string     name;
		PackageURL      url;
		ExtDependencies extDependencies;
		Projects        projects;
		Overrides       overrides;
		Installations   installations;
		Toolchains      toolchains;

	public:
		Package(PackageURL const& _url);

		auto getName() const -> std::string const&;
		void setName(std::string const& _name);

		auto getURL() const -> PackageURL const&;

		auto getExtDependencies() const -> ExtDependencies const&;
		auto getProjects() const -> Projects const&;
		auto accessProjects() -> Projects&;

		auto getOverrides() const -> Overrides const&;
		auto getInstallations() const -> Installations const&;
		auto getToolchains() const -> Toolchains const&;

		template <typename Node>
		void serialize(Node& node) {
			node["name"]            % name;
			node["extDependencies"] % extDependencies;
			node["projects"]        % projects;
			node["overrides"]       % overrides;
			node["installations"]   % installations;
			node["toolchains"]      % toolchains;
			for (auto& p : projects) {
				p.setPackagePath(url.getPath());
			}
		}

	};
}
