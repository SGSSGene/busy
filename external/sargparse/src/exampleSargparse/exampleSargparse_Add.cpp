#include <sargparse/sargparse.h>
#include <iostream>

namespace {

// example that emulates behavior as it is known with 'git add'

void addCallback();
auto myCommand = sargp::Command{"add", "add files", addCallback};
auto myFiles   = myCommand.Parameter<std::vector<std::string>>({}, "", "files to add", []{}, sargp::completeFile("", sargp::File::Multi));

void addCallback() {
	std::cout << "executing \"add\"" << std::endl;
	std::cout << "files:\n";
	for (auto const& f : *myFiles) {
		std::cout << "  - " << f << "\n";
	}
}
}
