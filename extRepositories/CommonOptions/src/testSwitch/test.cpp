/** \example src/testSwitch/test.cpp
 *
 * This examples shows how to parse options on the command line
 */
#include <commonOptions/commonOptions.h>
#include <selfTest/selfTest.h>
#include <iostream>
#include <vector>

namespace {
	auto swtPara1 = commonOptions::make_switch("para1", "just a simple switch");
}

int main(int argc, char** argv) {
	SELFTESTMAIN(argv); // Only for unittesting

	commonOptions::parse(argc, argv); // parsing options

	// print options
	std::cout << "para1: " << *swtPara1 << std::endl;

	return 0;
}

SELFTEST(Test1, "--para1", 0, "para1: 1\n", "")
SELFTEST(Test2, "-",       0, "para1: 0\n", "")

