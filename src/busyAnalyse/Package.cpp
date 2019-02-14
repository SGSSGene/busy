#include "Package.h"
#include <yaml-cpp/yaml.h>

namespace busy::analyse {

namespace fs = std::filesystem;

struct yaml_error : std::runtime_error {
	yaml_error(std::filesystem::path file, YAML::Node const& node)
		: runtime_error(file.string() + " in line " + std::to_string(node.Mark().line) + ":" + std::to_string(node.Mark().column) + " (" + std::to_string(node.Mark().pos) + ")")
	{}
	yaml_error(std::filesystem::path file, YAML::Node const& node, std::string msg)
		: runtime_error(file.string() + " in line " + std::to_string(node.Mark().line) + ":" + std::to_string(node.Mark().column) + " (" + std::to_string(node.Mark().pos) + ") - " + msg)
	{}

};


Package::Package(std::filesystem::path const& _path)
		: mPath { _path.lexically_normal()} {
	// read this package config
	auto path = mPath / "busy.yaml";
	auto node = YAML::LoadFile(path.string());

	if (not node["name"].IsScalar()) {
		throw yaml_error{path, node["name"], "expected 'name' as string"};
	}
	mName = node["name"].as<std::string>();

	if (node["projects"].IsDefined()) {
		if (not node["projects"].IsSequence()) {
			throw yaml_error{path, node["projects"], "expected 'projects' as sequence"};
		}
		auto projectNames = std::set<std::string>{};
		for(auto& p : fs::directory_iterator(mPath / "src")) {
			if (is_directory(p)) {
				projectNames.insert(p.path().filename());
			}
		}

		for (auto n : node["projects"]) {
			auto name = n["name"].as<std::string>();
			projectNames.erase(name);
			auto path = mPath / "src" / name;
			auto legacyIncludePaths = std::vector<fs::path>{};
			auto legacySystemLibraries    = std::set<std::string>{};

			for (auto e : n["legacy"]["includes"]) {
				legacyIncludePaths.push_back(mPath / e.as<std::string>());
				std::cout << "includes: " << mPath / e.as<std::string>() << "\n";
			}
			for (auto e : n["legacy"]["systemLibraries"]) {
				legacySystemLibraries.insert(e.as<std::string>());
			}

			mProjects.emplace_back(name, path, legacyIncludePaths, legacySystemLibraries);
		}
		for (auto const& p : projectNames) {
			mProjects.emplace_back(p, mPath / "src" / p, std::vector<fs::path>{}, std::set<std::string>{});
		}
	}

	// adding entries to package
	auto packages = std::vector<std::string>{};
	if (is_directory(mPath / external)) {
		for(auto& p : fs::directory_iterator(mPath / external)) {
			mPackages.emplace_back(p);
		}
	}

}

}
