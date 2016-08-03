#include "commands.h"

#include "Workspace.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <serializer/serializer.h>

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void showDep(std::string const&) {
	std::cout << "not implemented" << std::endl;
}

}

