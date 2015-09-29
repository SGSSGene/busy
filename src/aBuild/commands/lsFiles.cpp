#include "commands.h"

using namespace aBuild;

namespace commands {

std::set<std::string> getLsFiles(std::string const& _cwd) {
	utils::Cwd cwd(_cwd);
	utils::Process p1 ({"git", "ls-files"});
	utils::Process p2 ({"git", "ls-files", "-o", "--exclude-standard"});
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
void printLsFiles(std::string const& _cwd) {
	auto files = getLsFiles(_cwd);
	for (auto const& s : files) {
		std::cout << _cwd << "/" << s << std::endl;
	}
}

int listFiles() {
	if (not utils::fileExists("aBuild.json")) return EXIT_FAILURE;
	printLsFiles(".");
	auto projectDirs = utils::listDirs("packages", true);
	for (auto const& d : projectDirs) {
		auto path = std::string("packages/")+d;
		printLsFiles(path);
	}

	return EXIT_SUCCESS;
}

}
