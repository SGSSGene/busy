#pragma once

#include <condition_variable>
#include <chrono>
#include <iostream>
#include <thread>

namespace busy {

class ConsolePrinter {
	std::mutex mutex;
	std::chrono::milliseconds totalTime{0};
	int jobs{0};
	int totalJobs{0};
	std::string currentJob = "init";
	std::condition_variable cv;

	std::atomic_bool isRunning{true};
	std::thread thread;

public:
	ConsolePrinter(std::chrono::milliseconds _total, int _totalJobs)
	: totalTime {_total}
	, totalJobs {_totalJobs}
	, thread {[this]() {
		auto startTime = std::chrono::steady_clock::now();
		auto nextTime = startTime;
		while(isRunning) {
			auto g = std::unique_lock{mutex};
			nextTime += std::chrono::milliseconds{1000};
			cv.wait_until(g, nextTime);

			auto now = std::chrono::steady_clock::now();
			auto diff = duration_cast<std::chrono::milliseconds>(now - startTime);
			std::cout << "Jobs " << jobs << "/" << totalJobs << " - ETA " << (diff.count() / 1000.) << "s/" << (totalTime.count() / 1000.) << "s - " << currentJob << "\n";
		}
	}}

	{}

	~ConsolePrinter() {
		isRunning = false;
		cv.notify_one();
		thread.join();
	}
	void addTotalTime(std::chrono::milliseconds _total) {
		auto g = std::unique_lock{mutex};
		totalTime += _total;
	}
	void addJob(int _jobs) {
		auto g = std::unique_lock{mutex};
		jobs += _jobs;
	}
	void setCurrentJob(std::string _currentJob) {
		auto g = std::unique_lock{mutex};
		currentJob = std::move(_currentJob);
	}

};

}
