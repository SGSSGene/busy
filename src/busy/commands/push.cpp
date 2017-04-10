#include "commands.h"

#include "git.h"
#include "git-annex.h"
#include <busyUtils/busyUtils.h>
#include <busyConfig/busyConfig.h>
#include <iostream>
#include <threadPool/threadPool.h>

namespace commands {

void push(int jobs) {
	threadPool::ThreadPool<std::string> threadPool;
	threadPool.spawnThread([&](std::string const& path) {
		static std::mutex mutex;

		auto package = busyConfig::readPackage(path);
		if (not package.gitAnnex) {
			auto message = git::push(path);
			std::unique_lock<std::mutex> lock(mutex);
			std::cout << "pushed " << path << ": " << message << std::endl;
		} else {
			auto message = git::annex::sync_content(path);
			std::unique_lock<std::mutex> lock(mutex);
			std::cout << "pushed (git annex copy --all --to=origin --fast) " << path << ": " << message << std::endl;
		}
	}, jobs);

	auto allPackages = utils::listDirs("./extRepositories", true);

	std::vector<std::string> packagesToUpdate;
	for (auto& p : allPackages) {
		if (utils::fileExists("./extRepositories/" + p + "/.gitrepo")) continue;
		packagesToUpdate.push_back("./extRepositories/" + p);
	}
	packagesToUpdate.push_back(".");

	threadPool.queueContainer(packagesToUpdate);
	threadPool.wait();
}


}
