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
			auto p = process::Process{params};
			if (p.getStatus() == 0) {
				auto node = YAML::Load(p.cout());
				if (node["toolchains"].IsSequence()) {
					for (auto const& n : node["toolchains"]) {
						retList.emplace_back(n["name"].as<std::string>(), f.path());
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

auto getToolchainOptions(std::string_view _name, std::filesystem::path _path) -> std::map<std::string, std::vector<std::string>> {
	auto options = std::map<std::string, std::vector<std::string>>{};

	auto params = std::vector<std::string>{_path, "info"};
	auto p = process::Process{params};
	if (p.getStatus() != 0) throw std::runtime_error("error checking toolchain options");
	auto node = YAML::Load(p.cout());
	if (node["toolchains"].IsSequence()) {
		for (auto const& n : node["toolchains"]) {
			if (n["name"].as<std::string>() != _name) continue;

			auto nodeOptions = n["options"];
			if (not nodeOptions.IsMap()) continue;
			for (auto iter = nodeOptions.begin(); iter != nodeOptions.end(); ++iter) {
				auto& list = options[iter->first.as<std::string>()];
				if (iter->second.IsSequence()) {
					for (auto const& o : iter->second) {
						list.emplace_back(o.as<std::string>());
					}
				}
			}
		}
	}
	return options;
}


}
