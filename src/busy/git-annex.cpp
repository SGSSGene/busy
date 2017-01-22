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
namespace annex {

auto sync(std::string const& _cwd) -> std::string {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "annex", "sync"}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git annex sync on " + _cwd);
	}
	return TERM_GREEN "synced" TERM_RESET;
}

auto sync_content(std::string const& _cwd) -> std::string {
	setenv("LANGUAGE", "en_EN:en", 1);
	process::Process p({"git", "annex", "copy", "--all", "--to=origin", "--fast"}, _cwd);
	if (p.getStatus() != 0) {
		throw std::runtime_error("error running git annex sync on " + _cwd);
	}
	return TERM_GREEN "synced" TERM_RESET;
}


}
}

