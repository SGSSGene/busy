#include "git.h"

#include <algorithm>
#include <sstream>
#include <process/Process.h>


namespace git {

void clone(std::string const& _cwd, std::string const& _url, std::string const& _commit, std::string const& _dir) {
	process::Process p({"git", "clone", _url, "-b", _commit, _dir}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git clone on " + _cwd);
	}
}
void pull(std::string const& _cwd) {
	process::Process p({"git", "pull"}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git pull on " + _cwd);
	}
}
void push(std::string const& _cwd) {
	process::Process p({"git", "push"}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git push on " + _cwd);
	}
}
bool isDirty(std::string const& _cwd) {
	process::Process p({"git", "status", "--porcelain"}, _cwd);

	if (p.cout() == "" && p.cerr() == "") {
		return false;
	}
	return true;
}
void checkout(std::string const& _cwd, std::string const& _commit) {
	process::Process p({"git", "checkout", _commit}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git checkout on " + _cwd);
	}
}
auto getBranch(std::string const& _cwd) -> std::string {
	process::Process p({"git", "rev-parse", "--abbrev-ref", "HEAD"}, _cwd);
	std::string branch = p.cout();
	branch.pop_back();
	return branch;
}

int untrackedFiles(std::string const& _cwd) {
	process::Process p({"git", "status", "--porcelain"}, _cwd);
	auto const& s = p.cout();
	auto lines = utils::explode(s, "\n");
	int ct = 0;
	for (auto const& l : lines) {
		if (utils::isStartingWith(l, "?? ")) {
			ct += 1;
		}
	}
	return ct;
}

int changedFiles(std::string const& _cwd) {
	process::Process p({"git", "status", "--porcelain"}, _cwd);
	auto const& s = p.cout();
	auto lines = utils::explode(s, "\n");
	int ct = 0;
	for (auto const& l : lines) {
		if (not utils::isStartingWith(l, "?? ")) {
			ct += 1;
		}
	}
	return ct;
}

int commitsAhead(std::string const& _cwd) {
	process::Process p({"git", "status", "-u", "no"}, _cwd);
	auto const& s = p.cout();
	auto lines = utils::explode(s, "\n");
	auto pos = lines[1].find("up-to-date");
	if (pos != std::string::npos) {
		return 0;
	}
	auto posBy     = lines[1].rfind("by") + 3;
	auto posCommit = lines[1].rfind("commit") - 1;
	auto numberAsStr = lines[1].substr(posBy, posCommit);

	int ct = 0;
	std::stringstream ss;
	ss << numberAsStr;
	ss >> ct;
	return ct;
}


}
