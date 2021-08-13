#pragma once

#include <string>
#include <typeinfo>

auto demangle(std::type_info const& ti) -> std::string;

template<typename T>
auto demangle(T const& t) -> std::string {
    return demangle(typeid(t));
}

template<typename T>
auto demangle() -> std::string {
    return demangle(typeid(T));
}

