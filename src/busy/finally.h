#pragma once

#include <functional>

namespace busy {

class finally {
	std::function<void()> cb;
public:
	template <typename CB>
	finally(CB _cb)
	: cb{_cb}
	{}

	~finally() {
		if (cb) {
			cb();
		}
	}

	finally()                          = default;
	finally(finally const&)            = default;
	finally(finally&&)                 = default;
	finally& operator=(finally const&) = default;
	finally& operator=(finally&&)      = default;
};

}
