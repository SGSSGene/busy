#pragma once

#include <algorithm>

namespace estd {
	template<typename T, typename T2>
	auto find(T& vec, T2 const& val) -> typename T::iterator {
		return std::find(vec.begin(), vec.end(), val);
	}
}
