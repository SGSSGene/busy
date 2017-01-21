#include "commands.h"

#include "Workspace.h"
#include "git.h"

#include <busyUtils/busyUtils.h>
#include <iostream>

using namespace busy;

namespace commands {

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"


void status() {
	Workspace ws;

	std::cout << "current buildmode: " << ws.getSelectedBuildMode() << std::endl;
	std::cout << "current toolchain: " << ws.getSelectedToolchain() << std::endl;

	auto validPackages = ws.getPackages();
	if (validPackages.size() > 0) {
		int longestName = 0;
		for (auto const& p : validPackages) {
			longestName = std::max(longestName, int(p.getName().size()));
		}

		std::cout << "Status of external Repositories: " << std::endl;
		for (auto const& p : validPackages) {
			auto path = std::string("extRepositories/") + p.getName();
			if (&p == &validPackages.front()) { // first repository is always root
				path = "./.";
			}
			if (&p != &validPackages.back() and (not utils::dirExists("./extRepositories") or not utils::dirExists(path))) {
				std::cout << "  " << p.getName();
				for (int i = p.getName().size(); i < longestName; ++i) {
					std::cout << " ";
				}
				std::cout << " - repository not cloned" << std::endl;
			} else {
				auto untracked = git::untrackedFiles(path);
				auto changed   = git::changedFiles(path);
				auto ahead     = git::commitsAhead(path);
				if (untracked != 0 or changed != 0 or ahead != 0) {
					std::cout << "  " << p.getName();
					for (int i = p.getName().size(); i < longestName; ++i) {
						std::cout << " ";
					}

					if (untracked > 0) std::cout << " - " TERM_RED "untracked: " << untracked << TERM_RESET;
					else std::cout << " - untracked: " << untracked;
					if (changed > 0) std::cout << ", " TERM_RED "changed: " << changed << TERM_RESET;
					else std::cout << ", changed: " << changed;
					if (ahead > 0) std::cout << ", " TERM_GREEN "ahead: " << ahead << TERM_RESET << std::endl;
					else std::cout << ", ahead: " << ahead << std::endl;
				} else {
					std::cout << "  " << p.getName();
					for (int i = p.getName().size(); i < longestName; ++i) {
						std::cout << " ";
					}
					std::cout << " - up-to-date" << std::endl;

				}
			}
		}
	}
	std::vector<Package const*> duplicatedPackages;
	for (auto iter1 = validPackages.begin(); iter1 != validPackages.end(); ++iter1) {
		for (auto iter2 = validPackages.begin(); iter2 != validPackages.end(); ++iter2) {
			if (iter1 != iter2 and iter1->getName() == iter2->getName()) {
				duplicatedPackages.push_back(&*iter1);
				break;
			}
		}
	}
	if (duplicatedPackages.size() > 0) {
		std::cout << "\n" << TERM_RED << "Warning duplicated packages found:\n" << TERM_RESET;
		for (auto const& s : duplicatedPackages) {
			std::cout << " - " << s->getName() << " at " << s->getPath() << "\n";
		}
	}
}

}
