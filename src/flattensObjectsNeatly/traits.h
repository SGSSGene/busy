#pragma once

#include <type_traits>
#include <string_view>

namespace fon {

enum class ctor : uint8_t {};

template <typename Node, typename T>
struct has_serialize_function {
private:
    template <typename U>
    static auto test(int) -> decltype(std::declval<U>().serialize(std::declval<typename std::add_lvalue_reference<Node>::type>()), std::true_type());

    template <typename>
    static std::false_type test(...);
public:
    using type = decltype(test<T>(int(0)));
    enum { value = type::value };
};


template <typename Node, typename T>
constexpr static bool has_ser_v = has_serialize_function<Node, T>::value;


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

	return T{};
	if constexpr (std::is_constructible_v<T, ctor> and not std::is_arithmetic_v<T>) {
		return T{ctor{}};
	} else if constexpr (std::is_constructible_v<T>) {
		return T{};
	}
}

}
