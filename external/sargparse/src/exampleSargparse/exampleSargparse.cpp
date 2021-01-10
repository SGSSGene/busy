#include <sargparse/ArgumentParsing.h>
#include <sargparse/sargparse.h>
#include <iostream>

namespace {
auto printHelp  = sargp::Parameter<std::optional<std::string>>{{}, "help", "print this help add a string which will be used in a grep-like search through the parameters"};
}

int main(int argc, char** argv) {
	try {
		// create you own bash completion with this helper
		if (std::string(argv[argc-1]) == "--bash_completion") {
			auto hint = sargp::compgen(argc-2, argv+1);
			std::cout << hint << "\n";
			return EXIT_SUCCESS;
		}

		// parse the arguments (excluding the application name) and fill all parameters/flags/choices with their respective values
		sargp::parseArguments(argc-1, argv+1);
		if (printHelp) { // print the help
			std::cout << sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"});
			return EXIT_SUCCESS;
		}
		sargp::callCommands();
	} catch (std::exception const& e) {
		std::cout << "exception occurred: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
