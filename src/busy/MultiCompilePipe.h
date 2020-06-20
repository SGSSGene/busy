#pragma once
#include "CompilePipe.h"

namespace busy {
struct MultiCompilePipe {
	analyse::CompilePipe&    pipe;

	std::vector<std::thread> threads;
	bool                     isRunning{true};
	std::mutex               mutex;
	std::condition_variable  cv;
	std::size_t              idleThreads{0};

	MultiCompilePipe(analyse::CompilePipe& _pipe, int _threadCt)
	: pipe {_pipe}
	{
		threads.resize(_threadCt);
	}
private:
	template <typename CB>
	void singleThread(CB const& cb) {
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

