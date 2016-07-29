#include "commands.h"

#include "NeoWorkspace.h"
#include <iostream>

using namespace busy;

namespace commands {

/*static void runTest(std::string const& call) {
	std::cout << " • running " << call << "...";
	process::Process p({call});
	if (p.getStatus() == 0) {
		std::cout << " no errors" << std::endl;
	} else {
		std::cout << " errors: " << std::endl;
		if (p.cout().size() > 0) {
			std::cout << p.cout() << std::endl;
		}
		if (p.cerr().size() > 0) {
			std::cerr << p.cerr() << std::endl;
		}
	}
}*/
void test() {
	std::cout << "not implemented at the moment" << std::endl;
}

}
