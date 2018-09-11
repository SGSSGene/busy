#pragma once

#include "has_serialize_function.h"

namespace serializer {


	template <typename ...T>
	struct is_serializable_impl : public std::false_type {};

	template <typename Node, typename T>
	struct is_serializable_impl<Node, T>
		: public std::conditional<has_serialize_function<T, Node>::value, std::true_type, std::false_type>::type {};

	template <typename Node> struct is_serializable_impl<Node, bool> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, char> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, uint8_t> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, int8_t> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, uint16_t> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, int16_t> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, uint32_t> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, int32_t> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, uint64_t> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, int64_t> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, float> : public std::true_type {};
	template <typename Node> struct is_serializable_impl<Node, double> : public std::true_type {};

	template <typename Node> struct is_serializable_impl<Node, std::string> : public std::true_type {};

	template <typename Node, typename T1, typename T2> struct is_serializable_impl<Node, std::pair<T1, T2>>
		: public std::conditional<is_serializable_impl<Node, T1>::value and is_serializable_impl<Node, T2>::value, std::true_type, std::false_type>::type {};
	template <typename Node, typename T, int N> struct is_serializable_impl<Node, std::array<T, N>> : public is_serializable_impl<Node, T> {};
	template <typename Node, typename T> struct is_serializable_impl<Node, std::vector<T>> : public is_serializable_impl<Node, T> {};
	template <typename Node, typename T> struct is_serializable_impl<Node, std::list<T>>   : public is_serializable_impl<Node, T> {};
	template <typename Node, typename T> struct is_serializable_impl<Node, std::set<T>>    : public is_serializable_impl<Node, T> {};
	template <typename Node, typename Key, typename Value> struct is_serializable_impl<Node, std::map<Key, Value>>
		: public is_serializable_impl<Node, std::pair<Key, Value>> {};

	template <typename ...T>
	struct is_serializable : public is_serializable_impl<binary::SerializerNode, T...> {};

	template <typename Node, typename T>
	struct is_serializable<Node, T> : public is_serializable_impl<Node, T> {};



}
