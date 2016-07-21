#include "commands.h"
#include <iostream>

using namespace busy;

namespace commands {

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"


void status(std::string _buildMode) {
	Workspace ws(".");
	if (_buildMode != "") {
		if (_buildMode == "release" || _buildMode == "debug") {
			ws.accessConfigFile().setBuildMode(_buildMode);
		} else {
			throw "only \"release\" and \"debug\" are valid buildMode arguments";
		}
	}

	auto allToolchains = getAllToolchains(ws);

	Toolchain toolchain = allToolchains.rbegin()->second;
	std::string toolchainName = ws.accessConfigFile().getToolchain();
	if (allToolchains.find(toolchainName) != allToolchains.end()) {
		toolchain = allToolchains.at(toolchainName);
	}
	ws.accessConfigFile().setToolchain(toolchain.getName());

	std::cout << "current buildmode: " << ws.accessConfigFile().getBuildMode() << std::endl;
	std::cout << "current toolchain: " << ws.accessConfigFile().getToolchain() << std::endl;

	auto validPackages = ws.getAllValidPackages();
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


	auto missingPackages = ws.getAllMissingPackages();
	if (missingPackages.size() > 0) {
		std::cout << "\nPackages not cloned yet:" << std::endl;
		for (auto const& p : missingPackages) {
			std::cout << "  " << p.getName() << std::endl;
		}
	}

	auto invalidPackages = ws.getAllInvalidPackages();
	if (invalidPackages.size() > 0) {
		std::cout << "\nPackages are not valid:" << std::endl;
		for (auto const& p : invalidPackages) {
			std::cout << "  " << p << std::endl;
		}
	}

}

}
