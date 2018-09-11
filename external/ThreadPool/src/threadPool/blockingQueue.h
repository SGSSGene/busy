#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H

#include <atomic>
#include <condition_variable>
#include <queue>
#include <mutex>

namespace threadPool {

/**
 * This is a special QUEUE that will block on "pop" until element is available
 */
template<typename T>
class BlockingQueue final {
private:
	std::mutex m;
	std::condition_variable condition;
	std::queue<T> q;
	std::atomic_bool finish;
public:
	BlockingQueue()
		: finish(false) {
	}
	void queue(T const& t) {
		// Own scope for lock
		{
			std::unique_lock<std::mutex> lock(m);
			q.push(t);
		}
		condition.notify_one();
	}
	void queue(T&& t) {
		// Own scope for lock
		{
			std::unique_lock<std::mutex> lock(m);
			q.push(std::move(t));
		}

		condition.notify_one();
	}

	void queue(std::queue<T> const& _q) {
		// Own scope for lock
		{
			std::unique_lock<std::mutex> lock(m);
			while (_q.size() > 0) {
				q.push(_q.front());
				_q.pop();
			}
		}
		condition.notify_all();
	}
	void queue(std::queue<T>&& _q) {
		// Own scope for lock
		{
			std::unique_lock<std::mutex> lock(m);
			while (_q.size() > 0) {
				q.push(_q.front());
				_q.pop();
			}
		}
		condition.notify_all();
	}


	T dequeue() {
		std::unique_lock<std::mutex> lock(m);

		if (finish) {
			return T();
		}

		while (q.size() == 0) {
			condition.wait(lock);
			if (finish) {
				return T();
			}
		}

		T r = q.front();
		q.pop();
		return r;
	}
	/**
	 * Will force all waiting dequeue to leave by returning
	 * a default value of type T
	 */
	void forceFinish() {
		// Own scope for lock
		{
			std::unique_lock<std::mutex> lock(m);
			finish = true;
		}
		condition.notify_all();
	}
};

}
#endif
