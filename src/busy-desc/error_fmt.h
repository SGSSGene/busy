#pragma once

#include <fmt/format.h>
#include <stdexcept>

struct error_fmt : std::exception {
    std::string message;

    template <typename... Args>
    error_fmt(fmt::format_string<Args...> s, Args&&... args) {
        message = fmt::format(s, std::forward<Args>(args)...);
    }

    auto what() const noexcept -> char const* override {
        return message.c_str();
    }
};
