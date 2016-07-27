#include "NeoVisitor.h"

#include "NeoWorkspace.h"

#include <iostream>

namespace busy {
	NeoVisitor::NeoVisitor(NeoWorkspace& _workspace, std::string const& _name)
		: mWorkspace {_workspace}
		, mTarget    {_name}
	{
		setCppVisitor([] (NeoProject const*, std::string const& _file) {
		});
		setCVisitor([] (NeoProject const*, std::string const& _file) {
		});
		setProjectVisitor([] (NeoProject const* _project) {
		});
	}

	void NeoVisitor::visit() {
		auto projects = mWorkspace.getProjectAndDependencies(mTarget);

		struct Job {
			std::string name;
			std::function<void()> mAction;
			std::vector<std::string> mDependendOnOtherJobs;
			std::vector<Job*>        mDependendOnThisJob;
		};
		std::map<std::string, Job> mAllJobs;
		// create list of all changes
		for (auto const& project : projects) {
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
					job.mDependendOnOtherJobs.push_back(depProject->getFullName());
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
		std::vector<Job*> queuedJobs;
		for (auto& j : mAllJobs) {
			if (j.second.mDependendOnOtherJobs.size() == 0) {
				queuedJobs.push_back(&j.second);
			}
		}
		int jobCount = 0;
		while (not queuedJobs.empty()) {
			auto job = queuedJobs.back();
			queuedJobs.pop_back();
			job->mAction();
			for (auto otherJob : job->mDependendOnThisJob) {
				auto iter = std::find(otherJob->mDependendOnOtherJobs.begin(), otherJob->mDependendOnOtherJobs.end(), job->name);
				otherJob->mDependendOnOtherJobs.erase(iter);
				if (otherJob->mDependendOnOtherJobs.empty()) {
					queuedJobs.push_back(otherJob);
				}
			}
			jobCount += 1;
		}
		std::cout << "jobs done: " << jobCount << std::endl;
		std::cout << "jobs didn't finished:" << std::endl;
		for (auto& j : mAllJobs) {
			if (j.second.mDependendOnOtherJobs.size() != 0) {
				std::cout << "  - " << j.first << std::endl;
				for (auto& j2 : j.second.mDependendOnOtherJobs) {
					std::cout << "      * " << j2 << std::endl;
				}
			}
		}

		int cppCount = 0;
		for (auto const& project : projects) {
			cppCount += project->getCppFiles().size();
		}

		std::cout << "jobs: " << cppCount << " + " << projects.size() << " = " << cppCount + projects.size() << std::endl;
	}
}
