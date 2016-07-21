#include "commands.h"
#include <iostream>
#include <threadPool/threadPool.h>
using namespace busy;

namespace commands {

void push() {
	threadPool::ThreadPool<std::string> threadPool;
	threadPool.spawnThread([&](std::string const& path) {
		static std::mutex mutex;
		auto message = git::push(path);
		{
			std::unique_lock<std::mutex> lock(mutex);
			std::cout << "pushed " << path << ": " << message << std::endl;
		}

	}, 4);

	auto allPackages = utils::listDirs("./extRepositories", true);
	for (auto& p : allPackages) { p = "./extRepositories/"+p; }
	allPackages.push_back(".");

	threadPool.queueContainer(allPackages);
	threadPool.wait();
}


}
