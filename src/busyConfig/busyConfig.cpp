#include "busyConfig.h"

#include <flattensObjectsNeatly/flattensObjectsNeatly.h>
#include <flattensObjectsNeatly/filesystem.h>

#include <iostream>

namespace busyConfig {

auto readPackage(std::filesystem::path path) -> Package {
	std::map<std::string, YAML::Node> unused;

	path /= "busy.yaml";
	auto node = YAML::LoadFile(path.string());
	auto package = fon::yaml::deserialize<Package>(node);

	namespace fs = std::filesystem;

	// adding entries to package
	if (fs::status(path / Package::external).type() == fs::file_type::directory) {
		for(auto& p : fs::directory_iterator(path / Package::external)) {
			package.packages.push_back(p.path().string());
		}
	}



	return package;

}

}
