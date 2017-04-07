#include "CallbackHandler.h"

#include "Singleton.h"

namespace commonOptions {
	CallbackOnExitHandler::CallbackOnExitHandler(std::function<void()> _func)
		: mFunc {_func}
	{}
	CallbackOnExitHandler::~CallbackOnExitHandler() {
		if (mFunc) {
			mFunc();
		}
	}

}
