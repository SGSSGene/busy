/** \example src/exampleSimpleOption/test.cpp
 *
 * This examples shows how to parse options on the command line
 */
#include <commonOptions/commonOptions.h>
#include <iostream>
#include <vector>

namespace {
	auto optPara1 = commonOptions::make_option("para1", 0,         "this is an int");
	auto optPara2 = commonOptions::make_option("para2", 0.,        "this is a double");
	auto optPara3 = commonOptions::make_option("para3", "",        "this is a string");
}


int main(int argc, char** argv) {

	commonOptions::parse(argc, argv); // parsing options

	// print options
	std::cout << "para1: " << *optPara1 << std::endl;
	std::cout << "para2: " << *optPara2 << std::endl;
	std::cout << "para3: " << *optPara3 << std::endl;

	{
		auto optPara4 = commonOptions::make_option("a.b.para4", 0,        "this is a string");
		std::cout << "para4: " << *optPara4 << std::endl;
	}

	{
		auto optPara4 = commonOptions::make_option("a.b.para4", 0,        "this is a string");
		std::cout << "para4: " << *optPara4 << std::endl;
		auto optPara4_t = commonOptions::make_option("a.b.para4", 0,        "this is a string");
		std::cout << "para4_t: " << *optPara4 << std::endl;
	}

	auto unmatched = commonOptions::getUnmatchedParameters();
	if (unmatched.size() > 0) {
		std::cout << "Unknown parameters: \n";
		for (auto u : unmatched) {
			std::cout << u << "\n";
		}
	}


	return EXIT_SUCCESS;
}
