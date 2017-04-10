#include "busyConfig.h"
#include <serializer/serializer.h>
#include <iostream>

namespace busyConfig {
	auto readPackage(std::string const& _path) -> Package {
		std::map<std::string, YAML::Node> mUnused;

		Package package;
		auto file = _path + "/busy.yaml";
		serializer::yaml::read(file , package, &mUnused);

		if (not mUnused.empty()) {
			std::cerr << "unused options in " << file << std::endl;
			for (auto const& s : mUnused) {
				for (auto const& t : s.second) {
					if (s.first != "") {
						std::cerr << "unused: " << s.first << "." << t.first.as<std::string>() << std::endl;
					} else {
						std::cerr << "unused: " << t.first.as<std::string>() << std::endl;
					}
				}
			}
		}


		return package;

	}

}
