#pragma once

#include "CompilePipe.h"
#include "utils.h"

#include <condition_variable>
#include <thread>

namespace busy {

struct MultiCompilePipe {
	CompilePipe&    pipe;

	std::vector<std::thread> threads;
	bool                     isRunning{true};
	std::mutex               mutex;
	std::condition_variable  cv;
	std::size_t              idleThreads{0};
	bool                     compileError{false};

	MultiCompilePipe(CompilePipe& _pipe, int _threadCt)
	: pipe {_pipe}
	{
		threads.resize(_threadCt);
	}
private:
	template <typename CB>
	void singleThread(CB const& cb) {
		try {
		while(true) {
			auto g = std::unique_lock{mutex};
			if (not isRunning) return;
			if (not pipe.empty()) {
				auto work = pipe.pop();
				g.unlock();
				pipe.dispatch(work, cb);
				cv.notify_all();
			} else {
				idleThreads += 1;
				if (idleThreads == threads.size()) {
					isRunning = false;
					cv.notify_all();
					return;
				}
				cv.wait(g);
				idleThreads -= 1;
			}
		}
		} catch(CompileError const& e) {
			auto g = std::unique_lock{mutex};
			compileError = true;
			isRunning = false;
			cv.notify_all();
		}
	}
public:
	template <typename CB>
	void work(CB cb) {
		for (auto& thread : threads) {
			thread = std::thread{[this, cb]() {
				singleThread(cb);
			}};
		}
		cv.notify_all();
	}
	void join() {
		for (auto& thread : threads) {
			thread.join();
		}
	};
};

}
