#include "busy-lib/error_fmt.h"
#include "h.h"
#include <busy-lib/h.h>
#include <cassert>

int main() {
    auto e = error_fmt3{"{} {}", "Hello", "World"};
    assert(e.what() == std::string_view{"Hello World"});
}
