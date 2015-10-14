#pragma once

#include "utils.h"
#include "Process.h"

namespace git {

inline void clone(std::string const& _cwd, std::string const& _url, std::string const& _commit, std::string const& _dir) {
	utils::Process p({"git", "clone", _url, "-b", _commit, _dir}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git clone");
	}
}
inline void pull(std::string const& _cwd) {
	utils::Process p({"git", "pull"}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git clone");
	}
}
inline void push(std::string const& _cwd) {
	utils::Process p({"git", "push"}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git clone");
	}
}
inline bool isDirty(std::string const& _cwd) {
	utils::Process p({"git", "status", "--porcelain"}, _cwd);

	if (p.cout() == "" && p.cerr() == "") {
		return false;
	}
	return true;
}
inline void checkout(std::string const& _cwd, std::string const& _commit) {
	utils::Process p({"git", "checkout", _commit}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git clone");
	}
}
inline std::string getBranch(std::string const& _cwd) {
	utils::Process p({"git", "rev-parse", "--abbrev-ref", "HEAD"}, _cwd);
	std::string branch = p.cout();
	return branch;
}

}
