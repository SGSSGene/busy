#pragma once

#include <type_traits>
#include <string_view>

namespace fon {

enum class ctor : uint8_t {};

template <typename Node, typename T>
concept has_ser_v = requires(Node node, T t) {
    { t.serialize(node) };
};

template <typename Node, typename T>
concept has_reflect_v = requires(Node node, T t) {
    { T::reflect(node, t) };
};

// use as is_same_base<std::vector, T>::type
template <template <typename...> typename T1, typename T2>
struct is_same_base : std::false_type {};

template <template <typename...> typename T, typename ...Args>
struct is_same_base<T, T<Args...>> : std::true_type{};

template <template <typename...> typename T1, typename T2>
constexpr static bool is_same_base_v = is_same_base<T1, std::decay_t<T2>>::value;

template <typename T, typename ...Args>
constexpr static bool is_any_of_v = (std::is_same_v<T, Args> or...);

template <typename T>
auto getEmpty() -> T {
    static_assert(std::is_constructible_v<T, ctor> or std::is_constructible_v<T>,
        "object is not constructible");

    if constexpr (std::is_constructible_v<T, ctor> and not std::is_arithmetic_v<T>) {
        return T{ctor{}};
    } else if constexpr (std::is_constructible_v<T>) {
        return T{};
    } else {
        return T{};
    }
}

}
