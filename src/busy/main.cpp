#include <fmt/format.h>
#include <stdexcept>

void clice_main(int argc, char const* const* argv);
void app_main();

int main(int argc, char const* const* argv) {
    try {
        clice_main(argc, argv);
        app_main();
    } catch (std::exception const& e) {
        fmt::print("{}\n", e.what());
    } catch (char const* p) {
        fmt::print("{}\n", p);
    }
}
