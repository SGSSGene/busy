#include "toolchains.h"

#include "config.h"

#include <process/Process.h>
#include <yaml-cpp/yaml.h>

#include <iostream>

namespace busy {

auto searchForToolchains(std::filesystem::path _path) -> std::vector<std::tuple<std::string, std::filesystem::path>> {
	auto retList = std::vector<std::tuple<std::string, std::filesystem::path>>{};

	_path /= global_toolchainDir;

	if (exists(_path)) {
		for (auto f : std::filesystem::directory_iterator{_path}) {
			auto params = std::vector<std::string>{f.path(), "info"};
			auto canonical_relative_path = relative(f.path());
			auto p = process::Process{params};
			if (p.getStatus() == 0) {
				auto node = YAML::Load(p.cout());
				if (node["toolchains"].IsSequence()) {
					for (auto const& n : node["toolchains"]) {
						retList.emplace_back(n["name"].as<std::string>(), canonical_relative_path);
					}
				}
			}
		}
	}


	return retList;
}

auto searchForToolchains(std::vector<std::filesystem::path> const& _paths) -> std::map<std::string, std::filesystem::path> {
	auto retList = std::map<std::string, std::filesystem::path>{};
	for (auto const& p : _paths) {
		for (auto const& [name, path] : searchForToolchains(p)) {
			retList[name] = path;
		}
	}
	return retList;
}

}
