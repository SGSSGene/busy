#pragma once

#include <serializer/serializer.h>
#include "utils.h"

namespace aBuild {

	using Dependencies = std::vector<std::string>;
	using DepLibraries = std::vector<std::string>;

	struct ProjectLegacy {
		std::vector<std::string> includes;

		template<typename Node>
		void serialize(Node& node) {
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
			: packagePath { "." }
			, noWarnings  { false }
		{}

		template<typename Node>
		void serialize(Node& node) {
			node["path"]                 % path;
			node["dependencies"]         % dependencies;
			node["optionalDependencies"] % optionalDependencies;
			node["type"]                 % type                 or getDefaultTypeByName();
			node["legacy"]               % legacy;
			node["depLibraries"]         % depLibraries;
			node["noWarnings"]           % noWarnings           or false;
		}
		void set(std::string const& _name) {
			path = _name;
			type = getDefaultTypeByName();
		}

		auto getType() const -> std::string const& {
			return type;
		}
		/** tries to determine type by name
		 *  name starting with "test" are executables
		 *  everything else is a library
		 */
		auto getDefaultTypeByName() const -> std::string {
			if (utils::isStartingWith(path, "test")) {
				return "executable";
			}
			return "library";
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
