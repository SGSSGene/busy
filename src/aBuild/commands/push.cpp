#include "commands.h"
#include <iostream>
#include <threadPool/threadPool.h>
using namespace aBuild;

namespace commands {

void push() {
	threadPool::ThreadPool<std::string> threadPool;
	threadPool.spawnThread([&](std::string const& path) {
		static std::mutex mutex;
		{
			std::unique_lock<std::mutex> lock(mutex);
			std::cout << "pushing " << path << std::endl;
		}
		git::push(path);
	}, 4);

	auto allPackages = utils::listDirs("./packages", true);
	for (auto& p : allPackages) { p = "./packages/"+p; }
	allPackages.push_back(".");

	threadPool.queueContainer(allPackages);
	threadPool.wait();
}


}
