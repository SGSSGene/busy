#pragma once

#include <jsonSerializer/jsonSerializer.h>
#include "utils.h"

namespace aBuild {

	using Dependencies = std::vector<std::string>;
	using DepLibraries = std::vector<std::string>;

	struct ProjectLegacy {
		std::vector<std::string> includes;

		void serialize(jsonSerializer::Node& node) {
			node["includes"] % includes;
		}
	};


	class Project {
	private:
		std::string  path;
		std::string  packagePath;
		Dependencies dependencies;
		Dependencies optionalDependencies;
		DepLibraries depLibraries;
		bool         noWarnings;
		std::string  type;
		mutable std::vector<std::string> cppFiles;
		mutable std::vector<std::string> cFiles;

		ProjectLegacy legacy;

	public:
		Project()
			: packagePath {"."} {}

		void serialize(jsonSerializer::Node& node) {
			node["path"]                 % path;
			node["dependencies"]         % dependencies;
			node["optionalDependencies"] % optionalDependencies;
			node["type"]                 % type;
			node["legacy"]               % legacy;
			node["depLibraries"]         % depLibraries;
			node["noWarnings"]           % noWarnings           or false;
		}
		void set(std::string const& _name, std::string const& _type) {
			path = _name;
			type = _type;
		}

		auto getType() const -> std::string const& {
			return type;
		}
		auto getLegacy() const -> ProjectLegacy const& {
			return legacy;
		}
		auto getName() const -> std::string {
			auto l = utils::explode(getPath(), "/");
			return l[l.size()-1];
		}
		auto getPath() const -> std::string const& {
			return path;
		}
		auto getPackagePath() const -> std::string const& {
			return packagePath;
		}
		void setPackagePath(std::string const& s) {
			packagePath = s;
		}
		auto getDependencies() const -> Dependencies const& {
			return dependencies;
		}
		auto getOptionalDependencies() const -> Dependencies const & {
			return optionalDependencies;
		}
		auto getDepLibraries() const -> DepLibraries const& {
			return depLibraries;
		}
		bool getNoWarnings() const {
			return noWarnings;
		}
		auto getAllCppFiles() -> std::vector<std::string>& {
			if (cppFiles.empty()) {

				std::string fullPath = getPackagePath()+"/src/"+getPath()+"/";
				auto allFiles = utils::listFiles(fullPath, true);
				for (auto const& f : allFiles) {
					if (utils::isEndingWith(f, ".cpp")) {
						cppFiles.push_back(fullPath + f);
					}
				}
			}

			return cppFiles;
		}
		auto getAllCFiles() -> std::vector<std::string>& {
			if (cFiles.empty()) {

				std::string fullPath = getPackagePath()+"/src/"+getPath()+"/";
				auto allFiles = utils::listFiles(fullPath, true);
				for (auto const& f : allFiles) {
					if (utils::isEndingWith(f, ".c")) {
						cFiles.push_back(fullPath + f);
					}
				}
			}

			return cFiles;
		}


	};
}
