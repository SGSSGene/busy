#pragma once

#include "Project.h"
#include "utils.h"

namespace aBuild {

	class PackageURL {
	private:
		std::string name;
		std::string url;
		std::string branch;
		std::string path;
	public:
		PackageURL()
			: name {"_"}
			, path {"."} {
		}
		auto getName() const -> std::string const& {
			return name;
		}
		auto getURL() const -> std::string const& {
			return url;
		}
		auto getBranch() const -> std::string const& {
			return branch;
		}
		auto getPath() const -> std::string const& {
			return path;
		}
		bool operator<(PackageURL const& _other) const {
			return name < _other.name;
		}
		bool operator==(PackageURL const& _other) const {
			return name == _other.name;
		}
		void serialize(jsonSerializer::Node& node) {
			node["name"]   % name;
			node["url"]    % url;
			node["branch"] % branch;
			path = "packages/"+name;
		}
	};

	using ExtDependencies = std::vector<PackageURL>;
	using Projects        = std::vector<Project>;

	class Package {
	private:
		std::string     name;
		PackageURL      url;
		ExtDependencies extDependencies;
		Projects        projects;

	public:
		Package(PackageURL const& _url)
			: url {_url} {}

		void serialize(jsonSerializer::Node& node) {
			node["name"]            % name;
			node["extDependencies"] % extDependencies;
			node["projects"]        % projects;
			for (auto& p : projects) {
				p.setPackagePath(url.getPath());
			}
		}
		auto getName() const -> std::string const& { return name; }
		void setName(std::string const& _name) { name = _name; }

		auto getURL() const -> PackageURL const& { return url; }

		auto getExtDependencies() const -> ExtDependencies const& {
			return extDependencies;
		}
		auto getProjects() const -> Projects const& {
			return projects;
		}
		auto accessProjects() -> Projects& {
			return projects;
		}
	};
}
