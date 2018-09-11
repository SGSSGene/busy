#include <threadPool/threadPool.h>
#include <gtest/gtest.h>

using namespace threadPool;

TEST(TestThreadPool, SimpleQueue) {
	std::atomic_int ct(0);

	ThreadPool<int> threadPool;
	threadPool.spawnThread([&ct](int) { ++ct; }, 5);

	for (int i(0); i<10; ++i) {
		threadPool.queue(0);
	}
	threadPool.wait();

	EXPECT_EQ(ct, 10);
}
TEST(TestThreadPool, SimpleQueue2) {
	for (int j(0); j<100; ++j) {
		std::atomic_int ct(0);
		ThreadPool<int> threadPool;
		threadPool.spawnThread([&ct](int) { ++ct; }, 5);

		for (int i(0); i<10; ++i) {
			threadPool.queue(0);
		}
		threadPool.wait();

		EXPECT_EQ(ct, 10);
	}
}

TEST(TestThreadPool, RecursiveQueue) {
	std::atomic_int ct(0);

	ThreadPool<int> threadPool;
	threadPool.spawnThread([&ct, &threadPool](int _ct) {
		++ct;
		if (_ct > 0)
			threadPool.queue(_ct-1);
	}, 5);

	for (int i(0); i<10; ++i) {
		threadPool.queue(1);
	}
	threadPool.wait();

	EXPECT_EQ(ct, 20);
}

TEST(TestThreadPool, RecursiveQueue2) {
	for (int j(0); j<100; ++j) {

		std::atomic_int ct(0);
		ThreadPool<int> threadPool;
		threadPool.spawnThread([&ct, &threadPool](int _ct) {
			++ct;
			if (_ct > 0)
				threadPool.queue(_ct-1);
		}, 5);



		for (int i(0); i<10; ++i) {
			threadPool.queue(1);
		}
		threadPool.wait();

		EXPECT_EQ(ct, 20);
	}
}

