#pragma once

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct WorkQueue {
    struct Job {
        std::string              name;
        ssize_t                  blockingJobs{}; // Number of Jobs that are blocking this job
        std::function<void()>    job;
        std::vector<std::string> waitingJobs;    // Jobs that are waiting for this job
    };

    std::mutex                 mutex;
    std::condition_variable    cv;
    std::map<std::string, Job> allJobs;
    std::vector<std::string>   readyJobs;
    ssize_t                    jobsDone{};


    /* Inserts a job
     * \param name: name of this job a unique identifier
     * \param func: the function to execute to solve this job
     * \param blockingJobs: a list of jobs that have to be finished before this one (jobs that have to be executed before this one)
     */
    void insert(std::string name, std::function<void()> func, std::unordered_set<std::string> const& blockingJobs) {
        auto g = std::lock_guard{mutex};
        for (auto const& j : blockingJobs) {
            allJobs[j].waitingJobs.push_back(name);
        }
        auto& job = allJobs[name];
        if (!job.name.empty()) {
            throw std::runtime_error("duplicate job name \"" + name + "\", abort!");
        }
        job.name          = name;
        job.blockingJobs += ssize(blockingJobs);
        job.job           = std::move(func);

        if (blockingJobs.size() == 0) {
            readyJobs.emplace_back(name);
        }
    }

    bool processJob() {
        auto g = std::unique_lock{mutex};
        if (jobsDone == allJobs.size()) {
            return false;
        }

        if (!readyJobs.empty()) {
            auto last = readyJobs.back();
            readyJobs.pop_back();
            auto const& job = allJobs.at(last);
            g.unlock();
//            std::cout << "processing: " << job.name << "\n";
            job.job();
            finishJob(job.name);
        } else {
            cv.wait(g);
        }

        return true;
    }

private:
    void finishJob(std::string const& name) {
        auto g = std::lock_guard{mutex};
        for (auto j : allJobs.at(name).waitingJobs) {
            allJobs.at(j).blockingJobs -= 1;
            if (allJobs.at(j).blockingJobs == 0) {
                readyJobs.emplace_back(j);
            }
        }
        jobsDone += 1;
        cv.notify_all();
    }

public:
    void flush() {
        cv.notify_all();
    }
};
