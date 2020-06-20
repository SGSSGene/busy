#pragma once

#include "analyse.h"

#include <condition_variable>
#include <chrono>
#include <iostream>
#include <thread>

namespace busy {

class ConsolePrinter {
public:
	using File    = busy::analyse::File;
	using Project = busy::analyse::Project;
	using Variant = std::variant<File const*, Project const*>;

	using EstimatedTimes = std::unordered_map<Variant, std::chrono::milliseconds>;
private:

	std::mutex mutex;

	EstimatedTimes estimatedTimes;
	std::unordered_map<Variant, std::chrono::steady_clock::time_point> startTimes;

	std::chrono::milliseconds totalTime{};
	int totalJobs{};
	int jobs{0};
	std::string currentJob = "init";
	std::condition_variable cv;

	std::atomic_bool isRunning{true};
	std::thread thread;

	std::chrono::steady_clock::time_point const startTime;

public:
	ConsolePrinter(EstimatedTimes _estimatedTimes, std::chrono::milliseconds _totalTime)
	: estimatedTimes   {std::move(_estimatedTimes)}
	, totalTime {_totalTime}
	, totalJobs {estimatedTimes.size()}
	, startTime {std::chrono::steady_clock::now()}
	, thread {[this]() {
		auto nextTime = startTime;
		while(isRunning) {
			auto g = std::unique_lock{mutex};
			nextTime += std::chrono::milliseconds{1000};
			cv.wait_until(g, nextTime);
			print();
		}
	}}
	{}

	void print() {
		auto now = std::chrono::steady_clock::now();
		auto diff = duration_cast<std::chrono::milliseconds>(now - startTime);
		totalTime = std::max(diff, totalTime);
		std::cout << "Jobs " << jobs << "/" << totalJobs << "(" << startTimes.size() << ") - ETA " << (diff.count() / 1000.) << "s/" << (totalTime.count() / 1000.) << "s - " << currentJob << "\n";
	}

	~ConsolePrinter() {
		isRunning = false;
		cv.notify_one();
		thread.join();
	}

public:
	template <typename T>
	void startJob(T const* _key, std::string _currentJob) {
		auto start = std::chrono::steady_clock::now();
		auto g = std::unique_lock{mutex};
		startTimes.try_emplace(_key, start);
		currentJob = std::move(_currentJob);
		print();
	}
	template <typename T>
	auto finishedJob(T const* _key) -> std::chrono::milliseconds {
		auto g = std::unique_lock{mutex};
		auto stop = std::chrono::steady_clock::now();

		auto expectedTime = estimatedTimes.at(_key);
		auto startTime    = startTimes.at(_key);
		auto min_iter = begin(startTimes);
		for (auto iter = begin(startTimes); iter != end(startTimes); ++iter) {
			if (min_iter->second > iter->second) {
				min_iter = iter;
			}
		}
		auto this_iter = startTimes.find(_key);

		auto actualTime = std::chrono::duration_cast<std::chrono::milliseconds>(stop - startTime);
		if (this_iter == min_iter) {
			totalTime = totalTime + (actualTime - actualTime) / int(startTimes.size());
		}

		startTimes.erase(this_iter);

		jobs += 1;
		return actualTime;
	}

	template <typename T>
	void finishedJob(T const* _key, std::chrono::milliseconds _time) {
		auto g = std::unique_lock{mutex};
		auto iter = estimatedTimes.find(_key);
		if (iter == end(estimatedTimes)) {
			throw std::runtime_error("impossible error");
		}
		totalTime -= iter->second;
		totalTime += _time;
		jobs += 1;
	}

	void setCurrentJob(std::string _currentJob) {
		auto g = std::unique_lock{mutex};
		currentJob = std::move(_currentJob);
	}

};

}
