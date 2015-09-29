#include "commands.h"

using namespace aBuild;

namespace commands {

void pull() {
	auto allPackages = utils::listDirs("./packages", true);
	for (auto& p : allPackages) { p = "./packages/"+p; }
	allPackages.push_back(".");

	utils::runParallel<std::string>(allPackages, [](std::string const& file) {
		utils::Cwd cwd(file);
		if (git::isDirty()) {
			std::cout<<"ignore " << file << ": Dirty repository"<<std::endl;
		} else {
			git::pull();
		}
	});
}


}
