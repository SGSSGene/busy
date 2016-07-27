#include "commands.h"

#include "NeoWorkspace.h"

#include <iostream>

using namespace busy;

namespace commands {


namespace {
class TablePrint {
	std::vector<std::vector<std::string>> mEntries;
	std::vector<int> mColumnSize;

public:
	void add(std::vector<std::string> line) {
		for (int i(0); i < int(line.size()); ++i) {
			auto const& e = line.at(i);
			if (i >= int(mColumnSize.size())) {
				mColumnSize.emplace_back(0);
			}
			if (mColumnSize.at(i) < int(e.size())) {
				mColumnSize[i] = int(e.size());
			}
		}
		mEntries.emplace_back(std::move(line));
	}
	void print() {
		for (auto const& line : mEntries) {
			for (int i(0); i < int(line.size()); ++i) {
				auto const& e = line.at(i);
				printFilled(e, mColumnSize.at(i) + 1);
			}
			std::cout << std::endl;
		}
	}
private:
	void printFilled(std::string const& _str, int n) {
		int fillUp = n - int(_str.size());
		std::cout << _str;
		for (int i(0); i < fillUp; ++i) {
			std::cout << " ";
		}
	}
};

template <typename T>
void printList(std::string const& _title, std::vector<T> const& _list, std::function<void(T const&)> _func = [](T const& t) { std::cout << t; }) {
	if (_list.empty()) return;
	std::cout << _title << std::endl;
	for (auto const& s : _list) {
		std::cout << "  - ";
		_func(s);
		std::cout << std::endl;
	}
}
}

void info(std::vector<std::string> str) {

	NeoWorkspace ws;

	if (str.at(0) == "packageFolders" && str.size() == 1) {
		std::cout << "PackageFolders: " << std::endl;
		for (auto const& f : ws.getPackageFolders()) {
			std::cout << "  - " << f << std::endl;
		}
	} else if (str.at(0) == "packages" && str.size() == 1) {
		std::cout << "Packages:" << std::endl;
		for (auto const& package : ws.getPackages()) {
			std::cout << "  - " << package.getName() << std::endl;
		}
	} else if (str.at(0)  == "projects" && str.size() == 1) {
		TablePrint tp;
		tp.add({"project", "", "path", "has config entry"});
		for (auto package : ws.getPackages()) {
			for (auto project : package.getProjects()) {
				tp.add({project.getFullName(), "found at", project.getPath(), project.getHasConfigEntry() ? "true" : "false"});
			}
		}
		tp.print();
	} else if (str.at(0) == "package" && str.size() == 2) {
		auto package = ws.getPackage(str.at(1));

		std::cout << "Package: " << package.getName() << std::endl;
		std::cout << "  - Projects: " << std::endl;
		for (auto const& project : package.getProjects()) {
			std::cout << "    * " << package.getName() << "/" << project.getName() << std::endl;
		}
		std::cout << "  - External Repositories: " << std::endl;
		for (auto const& rep : package.getExternalPackageURLs()) {
			std::cout << "    * " << rep.name << " " << rep.url << " " << rep.branch << std::endl;
		}
	} else if (str.at(0) == "project" && str.size() == 2) {
		auto const& project = ws.getProject(str.at(1));
		std::cout << "Project: " << project.getFullName() << std::endl;

		std::cout << "Type: " << project.getType() << std::endl;
		std::cout << "has entry in busy.yaml: " << std::boolalpha << project.getHasConfigEntry() << std::endl;
		std::cout << "link whole archive: " << project.getWholeArchive() << std::endl;
		std::cout << "auto dependency discovery: " << project.getAutoDependenciesDiscovery() << std::endl;
		printList<NeoProject const*>("Dependencies:",  project.getDependencies(), [] (NeoProject const* const& p) { std::cout << p->getFullName(); });
		printList("Source Paths:",  project.getSourcePaths());
		printList("Include Paths:", project.getIncludePaths());
		printList("System Include Paths:", project.getSystemIncludePaths());
/*		printList("C-Files:",       project.getCFiles());
		printList("C++-Files:",     project.getCppFiles());
		printList("Include-Files:", project.getIncludeFiles());*/
//		printList("Defines:",       project.getDefines());
	} else if (str.at(0) == "compile") {
		std::cout << "available toolchains: " << std::endl;
		for (auto const& toolchain : ws.getToolchains()) {
			std::cout << "  - " << toolchain.first << std::endl;
		}
		std::cout << "picking system-gccc as toolchain" << std::endl;
		auto toolchain = ws.getToolchains().at("system-gcc");

		std::cout << "available flavors: " << std::endl;
		for (auto const& flavor : ws.getFlavors()) {
			std::cout << "  - " << flavor.first << std::endl;
		}
		std::cout << "picking busy/default flavor" << std::endl;

		auto flavor = ws.getFlavors().at("busy/default");

		auto projects = ws.getProjectAndDependencies(str.size() > 1?str.at(1):"");
/*		std::cout << "compiling: " << std::endl;
		for (auto const& project : projects) {
			std::string type = project->getType();
			if (flavor->isShared(project)) {
				type = "shared library";
			}
			std::cout << project->getFullName() << " compile as " << type << std::endl;
		}*/

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
					std::cout << "link: " << project->getFullName() << std::endl;
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
					std::cout << "compiling cpp file: " << file << std::endl;
				};
			}
			for (auto file : project->getCFiles()) {
				auto& job = mAllJobs[file];
				job.mAction = [=] {
					std::cout << "compiling c file: " << file << std::endl;
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
				std::cout << job1.first << " -> " << job2AsString << std::endl;
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
}
