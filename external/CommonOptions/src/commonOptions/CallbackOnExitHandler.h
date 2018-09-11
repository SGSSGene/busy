#pragma once

#include <functional>

namespace commonOptions {

class CallbackOnExitHandler {
	std::function<void()> mFunc;
public:
	CallbackOnExitHandler(std::function<void()> _func);
	~CallbackOnExitHandler();
};

}
