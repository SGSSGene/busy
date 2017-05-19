#include "Visitor.h"

#include "Workspace.h"

#include <iostream>
#include <threadPool/threadPool.h>

namespace busy {
	Visitor::Visitor(Workspace& _workspace, std::string const& _name)
		: mWorkspace (_workspace)
		, mTarget    {_name}
	{
		setCppVisitor([] (Project const*, std::string const&) {});
		setCVisitor([] (Project const*, std::string const&) {});
		setHVisitor([] (Project const*, std::string const&) {});
		setProjectVisitor([] (Project const*) {});
		setStatisticUpdateCallback([] (int, int) {});
	}

	void Visitor::visit(int _jobs, bool _visitHeaders) {
		mWorkspace.markProjectAsShared(mTarget);

		auto projects = mWorkspace.getProjectAndDependencies(mTarget);

		struct Job {
			std::string name;
			std::function<void()> mAction;
			std::vector<std::string> mDependendOnOtherJobs;
			std::vector<Job*>        mDependendOnThisJob;
		};
		std::map<std::string, Job> mAllJobs;
		// create list of all changes
		for (auto project : projects) {
			// adding the project/linking itself as jobs
			{
				auto& job = mAllJobs[project->getFullName()];
				job.mAction = [=] {
					mProjectVisitor(project);
				};
				// adding source files as dependencies
				for (auto file : project->getCppFiles()) {
					job.mDependendOnOtherJobs.push_back(file);
				}
				for (auto file : project->getCFiles()) {
					job.mDependendOnOtherJobs.push_back(file);
				}
				//!TODO not ready yet
				// if this is an executable or shared library add other libraries as dependency
				for (auto depProject : project->getDependencies()) {
					auto iter = std::find(projects.begin(), projects.end(), depProject);

					if (iter != projects.end()) {
						job.mDependendOnOtherJobs.push_back(depProject->getFullName());
					}
				}
			}
			// adding source files as jobs
			for (auto file : project->getCppFiles()) {
				auto& job = mAllJobs[file];
				job.mAction = [=] {
					mCppVisitor(project, file);
				};
			}
			for (auto file : project->getCFiles()) {
				auto& job = mAllJobs[file];
				job.mAction = [=] {
					mCVisitor(project, file);
				};
			}

			if (_visitHeaders) {
				for (auto file : project->getIncludeFiles()) {
					auto& job = mAllJobs[file];
					job.mAction = [=] {
						mHVisitor(project, file);
					};
				}
			}
		}
		// run over all jobs and set their names
		for (auto& job : mAllJobs) {
			job.second.name = job.first;
		}
		// run over all jobs and set mDependendOnThisJob variable
		for (auto& job1 : mAllJobs) {
			for (auto const& job2AsString : job1.second.mDependendOnOtherJobs) {
				auto& job2 = mAllJobs[job2AsString];
				job2.mDependendOnThisJob.push_back(&job1.second);
			}
		}
		auto maxJobs = int(mAllJobs.size());
		int doneJobs = 0;
		std::mutex mMutex;
		threadPool::ThreadPool<Job*> threadPool;
		threadPool.spawnThread([&] (Job* _job) {
			{
				std::lock_guard<std::mutex> lock(mMutex);
				mStatisticUpdate(doneJobs, maxJobs);
				doneJobs += 1;
			}
			_job->mAction();
			std::lock_guard<std::mutex> lock(mMutex);
			for (auto otherJob : _job->mDependendOnThisJob) {
				auto iter = std::find(otherJob->mDependendOnOtherJobs.begin(), otherJob->mDependendOnOtherJobs.end(), _job->name);
				otherJob->mDependendOnOtherJobs.erase(iter);
				if (otherJob->mDependendOnOtherJobs.empty()) {
					threadPool.queue(otherJob);
				}
			}
		}, _jobs);

		{
			std::lock_guard<std::mutex> lock(mMutex);
			for (auto& j : mAllJobs) {
				if (j.second.mDependendOnOtherJobs.size() == 0) {
					threadPool.queue(&j.second);
				}
			}
		}
		threadPool.wait();
		{
			std::lock_guard<std::mutex> lock(mMutex);
			mStatisticUpdate(doneJobs, maxJobs);
			doneJobs += 1;
		}

		//!TODO missing cycle detection
	}
}
