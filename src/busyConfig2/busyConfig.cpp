#include "busyConfig.h"

#include <flattensObjectsNeatly/fon.h>
#include <flattensObjectsNeatly/yaml.h>

#include <filesystem>

namespace busyConfig {

auto readPackage(std::string const& _path) -> Package {
	std::map<std::string, YAML::Node> unused;

	auto file = _path + "/busy.yaml";
	auto package = fon::yaml::deserialize<Package>(YAML::LoadFile(file));

	namespace fs = std::filesystem;
	// adding entries to package
	if (fs::status(_path + "/external").type() == fs::file_type::directory) {
		for(auto& p : fs::directory_iterator(_path + "/external")) {
			package.packages.push_back(p.path().string());
		}
	}



	return package;

}

}
