#include "commands.h"

using namespace busy;

namespace commands {

void clone(std::string const& _url, std::string const& _dir) {
	git::clone(".", _url, "master", _dir);
	utils::Cwd cwd {_dir};
	commands::build("");
	commands::test();
}

}

