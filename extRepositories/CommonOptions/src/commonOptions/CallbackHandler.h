#pragma once

#include "CallbackOnExitHandler.h"

#include <memory>

namespace commonOptions {
	using CallbackHandler = std::unique_ptr<CallbackOnExitHandler>;
}


