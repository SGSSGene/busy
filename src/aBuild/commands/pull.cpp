#include "commands.h"
#include <threadPool/threadPool.h>
#include <iostream>
using namespace aBuild;

namespace commands {

void pull() {
	threadPool::ThreadPool<std::string> threadPool;
	threadPool.spawnThread([&](std::string const& path) {
		static std::mutex mutex;
		if (git::isDirty(path)) {
			std::unique_lock<std::mutex> lock(mutex);
			std::cout<<"ignore " << path << ": Dirty repository"<<std::endl;
		} else {
			auto message = git::pull(path);
			{
				std::unique_lock<std::mutex> lock(mutex);
				std::cout << "pulled " << path << ": " << message << std::endl;
			}

		}
	}, 4);

	auto allPackages = utils::listDirs("./packages", true);
	for (auto& p : allPackages) { p = "./packages/"+p; }
	allPackages.push_back(".");

	threadPool.queueContainer(allPackages);
	threadPool.wait();
}


}
