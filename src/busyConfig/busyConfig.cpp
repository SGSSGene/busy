#include "busyConfig.h"
#include <busyUtils/busyUtils.h>
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

		// adding entries to package
		std::set<std::string> urls;
		for (auto const& u : package.extRepositories) {
			urls.insert(u.name);
		}
		if (utils::fileExists(_path + "/extRepositories")) {
			auto includedRepos = utils::listDirs(_path + "/extRepositories");
			for (auto const& s : includedRepos) {
				if (utils::fileExists(_path + "/extRepositories/" + s + "/.gitrepo")) {
					if (urls.count(s) == 0) {
						urls.insert(s);
						package.extRepositories.push_back(PackageURL{s, s, "master"});
					}
				}
			}
		}



		return package;

	}

}
