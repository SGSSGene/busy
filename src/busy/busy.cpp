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
}

int main(int argc, char const** argv) {
	try {
		if (std::string_view{argv[argc-1]} == "--bash_completion") {
			auto hint = sargp::compgen(argc-2, argv+1);
			fmt::print("{}", hint);
			return 0;
		}

		sargp::parseArguments(argc-1, argv+1);
		if (printHelp) {
			fmt::print("{}", sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"}));
			return 0;
		}
		sargp::callCommands();
		return EXIT_SUCCESS;
	} catch (busy::CompileError const& e) {
	} catch (std::exception const& e) {
		fmt::print(std::cerr, "exception {}\n", busy::utils::exceptionToString(e, 0));
		fmt::print(std::cerr, "{}", sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"}));
	}
	return EXIT_FAILURE;
}
