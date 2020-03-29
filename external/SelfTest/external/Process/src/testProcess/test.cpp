#include <process/Process.h>
#include <iostream>

int main(int argc, char** argv) {
	if (argc == 1) {
		process::Process p({argv[0], "para1"});
		if (p.getStatus() != 0) {
			std::cerr << "test failed" << std::endl;
			return EXIT_FAILURE;
		} else {
			std::cout << "test successful" << std::endl;
		}
		return EXIT_SUCCESS;
	} else if (argc == 2 && std::string(argv[1]) == "para1") {
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}
