#include <selfTest/selfTest.h>

SELFTEST(Test1, "0", 0, "", "")
SELFTEST(Test2, "1", 1, "", "")
SELFTEST(Test3, "2", 2, "", "")
SELFTEST(Test4, "", 255, "", "")
SELFTEST(Test5, "3", 0, "hello\n", "")
SELFTEST(Test6, "4", 0, "", "world\n")
SELFTEST(Test7, "5 6", 0, "", "")

int main(int argc, char** argv) {
	SELFTESTMAIN(argv);



	if (argc == 2) {
		if (std::string(argv[1]) == "0") {
			return 0;
		}
		if (std::string(argv[1]) == "1") {
			return 1;
		}
		if (std::string(argv[1]) == "2") {
			return 2;
		}
		if (std::string(argv[1]) == "3") {
			std::cout << "hello" << std::endl;
			return 0;
		}
		if (std::string(argv[1]) == "4") {
			std::cerr << "world" << std::endl;
			return 0;
		}
	}
	if (argc == 3) {
		if (std::string(argv[1]) == "5" && std::string(argv[2]) == "6") {
			return 0;
		}
	}

	return 255;
}

