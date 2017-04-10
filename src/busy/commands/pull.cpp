#include "commands.h"

#include "git.h"
#include "git-annex.h"
#include <busyUtils/busyUtils.h>
#include <busyConfig/busyConfig.h>
#include <iostream>
#include <threadPool/threadPool.h>

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

namespace {

void cloningExtRepositories(busyConfig::PackageURL url) {
	utils::mkdir(".busy/tmp");
	std::string repoName = std::string(".busy/tmp/repo_") + url.name + ".git";
	if (utils::fileExists(repoName)) {
		utils::rm(repoName, true, true);
	}
	std::cout << "cloning " << url.url << std::endl;
	git::clone(".", url.url, url.branch, repoName);
	auto configPackage = busyConfig::readPackage(repoName);
	utils::mv(repoName, std::string("extRepositories/") + url.name);
}

auto checkMissingDependencies() -> std::set<std::string> {
	std::set<std::string> checkedOutPaths;

	std::queue<busyConfig::PackageURL> queue;

	// read config file, set all other data
	auto configPackage = busyConfig::readPackage(".");
	for (auto x : configPackage.extRepositories) {
		queue.push(x);
	}

	if (not utils::fileExists("extRepositories")) {
		utils::mkdir("extRepositories");
	}

	while (not queue.empty()) {
		auto element = queue.front();
		queue.pop();

		if (not utils::fileExists("extRepositories/" + element.name + "/busy.yaml")) {
			cloningExtRepositories(element);
			checkedOutPaths.insert(element.name);
		}
		auto config = busyConfig::readPackage("extRepositories/" + element.name);
		for (auto x : config.extRepositories) {
			queue.push(x);
		}
	}
	return checkedOutPaths;
}
}


namespace commands {

namespace {
	class CPull {
	private:
		std::mutex mutex;
	public:
		void pull(std::string const& path) {
			if (git::isDirty(path, true)) {
				std::unique_lock<std::mutex> lock(mutex);
				std::cout << TERM_RED "ignore " << path << ": Dirty repository" TERM_RESET << std::endl;
			} else {
				auto package = busyConfig::readPackage(path);
				if (not package.gitAnnex) {
					auto message = git::pull(path);
					std::unique_lock<std::mutex> lock(mutex);
					std::cout << "pulled " << path << ": " << message << std::endl;
				} else {
					auto message = git::annex::sync(path);
					std::unique_lock<std::mutex> lock(mutex);
					std::cout << "pulled (git annex sync)" << path << ": " << message << std::endl;
				}
			}
		}
	};
}

void pull(int jobs) {
	CPull pull;
	// if possible pull current directory first
	pull.pull(".");

	// check for missing dependencies
	auto paths = checkMissingDependencies();

	// pull all other repositories
	threadPool::ThreadPool<std::string> threadPool;
	threadPool.spawnThread([&](std::string const& path) {
		pull.pull(path);
	}, jobs);

	auto allPackages = utils::listDirs("./extRepositories", true);

	std::vector<std::string> packagesToUpdate;
	for (auto const& p : allPackages) {
		if (paths.count(p) > 0) continue;
		if (utils::fileExists("./extRepositories/" + p + "/.gitrepo")) continue;
		packagesToUpdate.push_back("./extRepositories/" + p);
	}

	threadPool.queueContainer(packagesToUpdate);
	threadPool.wait();
}


}
