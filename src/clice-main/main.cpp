#include <clice/clice.h>

namespace {
auto cliHelp    = clice::Argument{ .arg    = {"--help"},
                                   .desc   = "prints the help page",
                                   .cb     = []{ std::cout << clice::generateHelp(); exit(0); },
                                 };
}

void clice_main(int argc, char const* const* argv) {
    if (auto failed = clice::parse(argc, argv); failed) {
        std::cerr << "parsing failed: " << *failed << "\n";
        exit(1);
    }
    if (auto ptr = std::getenv("CLICE_COMPLETION"); ptr) {
        exit(0);
    }
}
