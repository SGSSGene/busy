#pragma once

#include "blockingQueue.h"

#include <thread>
#include <atomic>
#include <pthread.h>

namespace threadPool {

template<typename T>
class ThreadPool final {
private:
	BlockingQueue<T> blockingQueue;
	mutable std::mutex countMutex;
	int        count;
	std::condition_variable queueIsEmpty;
	std::mutex threadCountMutex;
	int        threadCount;
	std::atomic_bool finish;
	std::condition_variable threadIsEmpty;

	std::vector<std::unique_ptr<std::thread>> mThreadList;

public:
	ThreadPool()
		: count(0)
		, threadCount(0)
		, finish(false)
	{}

	/*
	 * Will abort all threads and open jobs
	 */
	~ThreadPool() {
		std::unique_lock<std::mutex> lock(threadCountMutex);
		finish = true;
		blockingQueue.forceFinish();
		if (threadCount > 0) {
			threadIsEmpty.wait(lock);
		}
		for (auto& t : mThreadList) {
			t->join();
		}
	}

	/**
	 * This will queue a new object, but will not block
	 */
	void queue(T t) {
		std::unique_lock<std::mutex> lock(countMutex);
		++count;
		blockingQueue.queue(t);
	}

	/**
	 * This function will queue full range, but will not block
	 */
	template<typename C>
	void queueContainer(C const& c) {
		std::unique_lock<std::mutex> lock(countMutex);
		for (T const& t : c ) {
			++count;
			blockingQueue.queue(t);
		}
	}

	/**
	 * Wait till queue is empty
	 *
	 */
	void wait() {
		std::unique_lock<std::mutex> lock(countMutex);
		if (count > 0) {
			queueIsEmpty.wait(lock);
		}
	}

	int queueSize() const {
		std::unique_lock<std::mutex> lock(countMutex);
		return count;
	}

	/**
	 * Spawns new threads that can work on a job
	 * This function should only be called once
	 *
	 * @TODO multiple calls of this function will lead to more and more threads,
	 *       because they never exit.
	 * @TODO This function can only be called when there are no jobs in the queue
	 *
	 * @_f           function that the threads should execute
	 * @_threadCount number of threads that should be spawned
	 */
	void spawnThread(std::function<void(T t)> _f, int _threadCount) {
		std::unique_lock<std::mutex> lock(threadCountMutex);
		threadCount += _threadCount;

		for (int i(0); i < _threadCount; ++i) {
			auto ptr = new std::thread([this, _f]() {
				while (true) {
					T job = blockingQueue.dequeue();
					if (finish) {
						bool threadPoolEmpty;
						// Own scope for lock
						{
							std::unique_lock<std::mutex> lock(threadCountMutex);
							--threadCount;
							threadPoolEmpty = (threadCount == 0);
						}
						if (threadPoolEmpty) {
							threadIsEmpty.notify_one();
						}
						return;
					}
					_f(job);
					std::unique_lock<std::mutex> lock(countMutex);
					--count;
					if (count == 0) {
						queueIsEmpty.notify_one();
					}
				}
			});
			mThreadList.emplace_back(ptr);
		}
	}
};

}
