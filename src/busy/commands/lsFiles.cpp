#include "commands.h"
#include <iostream>

using namespace aBuild;

namespace commands {

std::set<std::string> getLsFiles(std::string const& _cwd) {
	utils::Cwd cwd(_cwd);
	process::Process p1 ({"git", "ls-files"});
	process::Process p2 ({"git", "ls-files", "-o", "--exclude-standard"});
	auto files1 = utils::explode(p1.cout(), "\n");
	auto files2 = utils::explode(p2.cout(), "\n");
	std::set<std::string> files;
	for (auto const& s : files1) {
		files.insert(s);
	}
	for (auto const& s : files2) {
		files.insert(s);
	}
	return files;
}
void printLsFiles(std::string const& _prefix, std::string const& _cwd) {
	auto files = getLsFiles(_cwd);
	for (auto const& s : files) {
		if (_cwd == ".") {
			std::cout << _prefix << s << std::endl;
		} else {
			std::cout << _prefix <<_cwd << "/" << s << std::endl;
		}
	}
}

int listFiles(std::string const& _relPath) {
	if (not utils::fileExists("busy.yaml")) {
		return EXIT_FAILURE;
	}

	printLsFiles(_relPath, ".");
	auto cwdDirs = utils::cwd();
	auto cwdDirsList = utils::explode(cwdDirs, "/");
	if (cwdDirsList[cwdDirsList.size()-1] == "extRepositories") {
		auto projectDirs = utils::listDirs(".", true);
		for (auto const& d : projectDirs) {
			auto path = d;
			printLsFiles(_relPath, path);
		}
	} else if (utils::fileExists("extRepositories")) {
		auto projectDirs = utils::listDirs("extRepositories", true);
		for (auto const& d : projectDirs) {
			auto path = std::string("extRepositories/")+d;
			printLsFiles(_relPath, path);
		}
	}

	return EXIT_SUCCESS;
}

}
