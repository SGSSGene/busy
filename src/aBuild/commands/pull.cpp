#include "commands.h"
#include <threadPool/threadPool.h>
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
			{
				std::unique_lock<std::mutex> lock(mutex);
				std::cout << "pulling " << path << std::endl;
			}
			git::pull(path);
		}
	}, 4);

	auto allPackages = utils::listDirs("./packages", true);
	for (auto& p : allPackages) { p = "./packages/"+p; }
	allPackages.push_back(".");

	threadPool.queueContainer(allPackages);
	threadPool.wait();
}


}
