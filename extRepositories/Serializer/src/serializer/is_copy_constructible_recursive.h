#pragma once

namespace serializer {

template <typename ...Args>
struct is_copy_constructible_recursive;


template <>
struct is_copy_constructible_recursive<> : std::true_type
{};


template <typename T>
struct is_copy_constructible_recursive<T>
	: std::integral_constant<bool,
	                         std::is_copy_constructible<T>::value>
{};

template <typename Arg0, typename ...Args>
struct is_copy_constructible_recursive<Arg0, Args...>
	: std::integral_constant<bool,
	                         is_copy_constructible_recursive<Arg0>::value
	                         and is_copy_constructible_recursive<Args...>::value>
{};


template <template<typename, typename...> class T, typename Arg0, typename ...Args>
struct is_copy_constructible_recursive<T<Arg0, Args...>>
	: std::integral_constant<bool,
	                         std::is_copy_constructible<T<Arg0, Args...>>::value
	                         and is_copy_constructible_recursive<Arg0>::value
	                         and is_copy_constructible_recursive<Args...>::value>
{};

}
