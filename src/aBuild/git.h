#pragma once

#include "utils.h"
#include "Process.h"

namespace git {

inline void clone(std::string const& _url, std::string const& _commit, std::string const& _dir) {
	std::cout << "cloning " << _url << std::endl;
	utils::Process p({"git", "clone", _url, "-b", _commit, _dir});;
}
inline void pull() {
	std::string call = std::string("git pull");
	system(call.c_str());
}
inline void push() {
	std::string call = std::string("git push");
	system(call.c_str());
}
inline bool isDirty() {
	utils::rm("abuild_dirty", false, true);
	std::string call = "test -n \"$(git status --porcelain)\" && (echo \"dirty\" > abuild_dirty)";
	utils::runProcess(call);
	bool exists = utils::fileExists("abuild_dirty");
	utils::rm("abuild_dirty", false, true);
	return exists;
}
inline void checkout(std::string const& _commit) {
	std::string call = "git checkout " + _commit;
	utils::runProcess(call);
}
inline std::string getBranch() {
	utils::rm("abuild_branch", false, true);
	std::string call = "git branch | grep \"*\" | cut -d \" \" -f 2 > abuild_branch";
	utils::runProcess(call);
	std::ifstream ifs("abuild_branch");
	std::string branch;
	std::getline(ifs, branch);
	utils::rm("abuild_branch", false, true);
	return branch;
}

}
