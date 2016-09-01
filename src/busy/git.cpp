#include "git.h"

#include <algorithm>
#include <sstream>
#include <process/Process.h>
#include <stdlib.h>
#include <iostream>

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"


namespace git {

void clone(std::string const& _cwd, std::string const& _url, std::string const& _commit, std::string const& _dir) {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "clone", _url, "-b", _commit, _dir}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git clone on " + _cwd);
	}
}

auto pull(std::string const& _cwd) -> std::string {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "pull", "--rebase=false"}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git pull on " + _cwd);
	}
	if (p.cout() == "Already up-to-date.\n") {
		return "Already up-to-date";
	}
	if (p.cout().find("Updating ") != std::string::npos) {
		auto pos1 = p.cout().find("Updating ");
		auto pos2 = p.cout().find("\n", pos1);
		return TERM_GREEN + p.cout().substr(pos1, pos2) + TERM_RESET;
	}
	return TERM_RED "no clue what happend" TERM_RESET + p.cerr() + p.cout();
}
auto push(std::string const& _cwd) -> std::string {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "push"}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git push on " + _cwd);
	}
	if (p.cerr() == "Everything up-to-date\n") {
		return "Everything up-to-date";
	}
	return TERM_GREEN "pushed master" TERM_RESET;
}
bool isDirty(std::string const& _cwd, bool _ignoreUntrackedFiles) {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "status", "--porcelain"}, _cwd);

	auto cout = p.cout();
	bool notUntracked = false;
	if (_ignoreUntrackedFiles) {
		auto lines = utils::explode(cout, "\n");
		for (auto const& l : lines) {
			if (not utils::isStartingWith(l, "?? ")) {
				notUntracked = true;
				break;
			}
		}
	} else {
		notUntracked = cout != "";
	}
	if (not notUntracked && p.cerr() == "") {
		return false;
	}
	return true;
}
void checkout(std::string const& _cwd, std::string const& _commit) {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "checkout", _commit}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git checkout on " + _cwd);
	}
}
auto getBranch(std::string const& _cwd) -> std::string {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "rev-parse", "--abbrev-ref", "HEAD"}, _cwd);
	std::string branch = p.cout();
	branch.pop_back();
	return branch;
}
auto getCurrentHash(std::string const& _cwd) -> std::string {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "rev-parse", "HEAD"}, _cwd);
	std::string branch = p.cout();
	branch.pop_back();
	return branch;
}
auto getConfig(std::string const& _cwd, std::string const& _option) -> std::string {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "config", _option}, _cwd);
	std::string value = p.cout();
	if (not value.empty()) {
		value.pop_back();
	}
	return value;

}



int untrackedFiles(std::string const& _cwd) {
	setenv("LANGUAGE", "en_EN:en", 1);
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
	setenv("LANGUAGE", "en_EN:en", 1);
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
	setenv("LANGUAGE", "en_EN:en", 1);
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
