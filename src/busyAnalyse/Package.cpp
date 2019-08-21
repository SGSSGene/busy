#include "Package.h"

#include <yaml-cpp/yaml.h>

namespace busy::analyse {

constexpr std::string_view external{"external"};

namespace fs = std::filesystem;

struct yaml_error : std::runtime_error {
	yaml_error(std::filesystem::path file, YAML::Node const& node)
		: runtime_error(file.string() + " in line " + std::to_string(node.Mark().line) + ":" + std::to_string(node.Mark().column) + " (" + std::to_string(node.Mark().pos) + ")")
	{}
	yaml_error(std::filesystem::path file, YAML::Node const& node, std::string msg)
		: runtime_error(file.string() + " in line " + std::to_string(node.Mark().line) + ":" + std::to_string(node.Mark().column) + " (" + std::to_string(node.Mark().pos) + ") - " + msg)
	{}

};


auto readPackage(std::filesystem::path const& _root, std::filesystem::path const& _path) -> std::vector<Project> {
	auto retProjects = std::vector<Project>{};

	// read this package configuration
	auto path = _root / _path / "busy.yaml";

	try {

		auto node = YAML::LoadFile(path.string());

		// add all project names based on directory entries
		auto projectNames = std::set<std::string>{};
		for(auto& p : fs::directory_iterator(_root / _path / "src")) {
			if (is_directory(p)) {
				projectNames.insert(p.path().filename());
			}
		}

		// scan all defined projects
		if (node["projects"].IsDefined()) {
			if (not node["projects"].IsSequence()) {
				throw yaml_error{path, node["projects"], "expected 'projects' as sequence"};
			}

			for (auto n : node["projects"]) {
				auto name = n["name"].as<std::string>();
				projectNames.erase(name);
				auto path = _path / "src" / name;
				auto legacyIncludePaths = std::vector<fs::path>{};
				auto legacySystemLibraries    = std::set<std::string>{};

				for (auto e : n["legacy"]["includes"]) {
					legacyIncludePaths.push_back(_path / e.as<std::string>());
				}
				for (auto e : n["legacy"]["systemLibraries"]) {
					legacySystemLibraries.insert(e.as<std::string>());
				}
				retProjects.emplace_back(name, _root, path, legacyIncludePaths, legacySystemLibraries);
			}
		}

		// add all projects that weren't defined in "projects" section of busy.yaml
		for (auto const& p : projectNames) {
			auto path = _path / "src" / p;
			retProjects.emplace_back(p, _root, path, std::vector<fs::path>{}, std::set<std::string>{});
		}


		// adding entries to package
		auto packages = std::vector<std::string>{};
		if (is_directory(_root / _path / external)) {
			for (auto& p : fs::directory_iterator(_root / _path / external)) {
				if (not is_symlink(p)) {
					auto projects = readPackage(_root, relative(p, _root));
					retProjects.insert(end(retProjects), begin(projects), end(projects));
				}
			}
		}
	} catch(...) {
		std::throw_with_nested(std::runtime_error{"failed loading " + path.string()});
	}
	return retProjects;
}


}
