#include <clice/clice.h>

namespace {
auto cliHelp    = clice::Argument{ .arg    = {"--help"},
                                   .desc   = "prints the help page",
                                   .cb     = []{ std::cout << clice::generateHelp(); exit(0); },
                                 };
}

int clice_main();

int main(int argc, char const* const* argv) {
    if (auto failed = clice::parse(argc, argv); failed) {
        std::cerr << "parsing failed: " << *failed << "\n";
        return -1;
    }
    if (auto ptr = std::getenv("CLICE_COMPLETION"); ptr) {
        return 0;
    }

    return clice_main();
}
