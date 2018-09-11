#pragma once

#include <type_traits>
namespace serializer {

	template <typename T, typename Arg1>
	struct has_serialize_function {
	private:
	    template <typename U>
	    static decltype(std::declval<U>().serialize(std::declval<typename std::add_lvalue_reference<Arg1>::type>()), void(), std::true_type()) test(int);

	    template <typename>
	    static std::false_type test(...);
	public:
	    using type = decltype(test<T>(int(0)));
	    enum { value = type::value };
};

}

