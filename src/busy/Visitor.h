#pragma once

#include <functional>
#include <string>

namespace busy {
	class Workspace;
	class Project;

	class Visitor {
		using FileVisitor      = std::function<void(Project const* _project, std::string const& _file)>;
		using ProjectVisitor  = std::function<void(Project const* _project)>;
		using StatisticUpdate = std::function<void(int _done, int _total)>;

		Workspace& mWorkspace;
		std::string   mTarget;

		FileVisitor     mCppVisitor;
		FileVisitor     mCVisitor;
		FileVisitor     mHVisitor;
		ProjectVisitor  mProjectVisitor;
		StatisticUpdate mStatisticUpdate;
	public:
		Visitor(Workspace& _ws, std::string const& _name);
		void setCppVisitor(FileVisitor _cppVisitor) { mCppVisitor = _cppVisitor; }
		void setCVisitor(FileVisitor _cVisitor) { mCVisitor = _cVisitor; }
		void setHVisitor(FileVisitor _hVisitor) { mHVisitor = _hVisitor; }
		void setProjectVisitor(ProjectVisitor _projectVisitor) { mProjectVisitor = _projectVisitor; }
		void setStatisticUpdateCallback(StatisticUpdate _update) { mStatisticUpdate = _update; }

		void visit(int _threads, bool _visitHeaders = false);
	};
}
