#pragma once

#include "CallbackHandler.h"

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace commonOptions {

class Singleton {
private:
	int id { 0 };
	std::vector<std::pair<int, std::function<void()>>> mCallbacksOnBeforeSave;
	std::vector<std::pair<int, std::function<void()>>> mCallbacksOnSave;
	std::vector<std::pair<int, std::function<void()>>> mCallbacksOnLoad;

	std::mutex mMutex;
public:
	static auto getInstance() -> Singleton&;

	auto register_on_before_save(std::function<void()> _callback) -> CallbackHandler;
	auto register_on_save(std::function<void()> _callback)        -> CallbackHandler;
	auto register_on_load(std::function<void()> _callback)        -> CallbackHandler;

	void signal_save();
	void signal_load();

};


}
