#include <span>
#include <vector>
#include <string_view>


int app(std::span<std::string_view> args);

int main(int argc, char const* const* argv) {
    auto args = std::vector<std::string_view>{};
    for (int i{0}; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return app(args);
}
