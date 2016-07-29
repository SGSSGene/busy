#include "commands.h"

#include "git.h"
#include <busyUtils/busyUtils.h>
#include <iostream>
#include <threadPool/threadPool.h>

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

namespace commands {

void pull() {
	threadPool::ThreadPool<std::string> threadPool;
	threadPool.spawnThread([&](std::string const& path) {
		static std::mutex mutex;
		if (git::isDirty(path)) {
			std::unique_lock<std::mutex> lock(mutex);
			std::cout << TERM_RED "ignore " << path << ": Dirty repository" TERM_RESET << std::endl;
		} else {
			auto message = git::pull(path);
			{
				std::unique_lock<std::mutex> lock(mutex);
				std::cout << "pulled " << path << ": " << message << std::endl;
			}

		}
	}, 4);

	auto allPackages = utils::listDirs("./extRepositories", true);
	for (auto& p : allPackages) { p = "./extRepositories/"+p; }
	allPackages.push_back(".");

	threadPool.queueContainer(allPackages);
	threadPool.wait();
}


}
