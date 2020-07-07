#include "utils.h"
#include "utils/utils.h"

#include <algorithm>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>
#include <sargparse/ArgumentParsing.h>
#include <sargparse/File.h>
#include <sargparse/Parameter.h>

namespace {
auto printHelp  = sargp::Parameter<std::optional<std::string>>{{}, "help", "print this help - add a string for  grep-like search"};
void help() {
	fmt::print("{}", sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"}));
}
}

int main(int argc, char const** argv) {
	if (std::string_view{argv[argc-1]} == "--bash_completion") {
		auto hint = sargp::compgen(argc-2, argv+1);
		fmt::print("{}", hint);
		return EXIT_SUCCESS;
	}

	try {
		sargp::parseArguments(argc-1, argv+1);
	} catch(std::exception const& e) {
		help();
		return EXIT_FAILURE;
	}

	if (printHelp) {
		help();
		return EXIT_SUCCESS;
	}
	try {
		sargp::callCommands();
	} catch (busy::CompileError const& e) {
	} catch (std::exception const& e) {
		fmt::print(std::cerr, "exception {}\n", busy::utils::exceptionToString(e, 0));
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
