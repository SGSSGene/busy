#pragma once

#include "convert.h"

#include <chrono>

namespace fon {

// convertible
template <typename Node, class Clock, class Duration>
struct convert<Node, std::chrono::time_point<Clock, Duration>> {
	static constexpr Type type = Type::Convertible;
	struct Infos {
		template <typename Node2>
		static void convert(Node2& node, std::chrono::time_point<Clock, Duration>& obj) {
			auto val = obj.time_since_epoch().count();
			node % val;
			obj = std::chrono::time_point<Clock, Duration>(Duration(val));
		}
	};
	convert(Node&, std::chrono::time_point<Clock, Duration>&) {}
};


}
