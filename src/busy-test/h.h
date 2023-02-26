#pragma once

#include <fmt/format.h>
#include <stdexcept>
#include <iostream>

struct error_fmt2 : std::runtime_error {
    template <typename... Args>
    error_fmt2(fmt::format_string<Args...> s, Args&&... args)
        : std::runtime_error{fmt::format(s, std::forward<Args>(args)...)} {
        std::cout << "hallo\n";
    }
};
