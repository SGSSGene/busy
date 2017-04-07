/** \example src/testSimpleOption/test.cpp
 *
 * This examples shows how to parse options on the command line
 */
#include <commonOptions/commonOptions.h>
#include <selfTest/selfTest.h>
#include <iostream>
#include <vector>

namespace {
	auto optPara1 = commonOptions::make_option("para1", 0,         "this is an int");
	auto optPara2 = commonOptions::make_option("para2", 0.,        "this is a double");
	auto optPara3 = commonOptions::make_option("para3", "",        "this is a string");
}


int main(int argc, char** argv) {
	SELFTESTMAIN(argv); // Only for unittesting

	commonOptions::parse(argc, argv); // parsing options

	// print options
	std::cout << "para1: " << *optPara1 << std::endl;
	std::cout << "para2: " << *optPara2 << std::endl;
	std::cout << "para3: " << *optPara3 << std::endl;

	return 0;
}

SELFTEST(Test1, "--para1 10 --para2 20. --para3 hi", 0, "para1: 10\npara2: 20\npara3: hi\n", "")
SELFTEST(Test2, "--para1 20 --para2 22. --para3 hello", 0, "para1: 20\npara2: 22\npara3: hello\n", "")


