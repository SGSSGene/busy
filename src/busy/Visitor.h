#pragma once

#include <functional>
#include <string>

namespace busy {
	class Workspace;
	class Project;

	class Visitor {
		using CppVisitor      = std::function<void(Project const* _project, std::string const& _file)>;
		using CVisitor        = std::function<void(Project const* _project, std::string const& _file)>;
		using ProjectVisitor  = std::function<void(Project const* _project)>;
		using StatisticUpdate = std::function<void(int _done, int _total)>;

		Workspace& mWorkspace;
		std::string   mTarget;

		CppVisitor      mCppVisitor;
		CVisitor        mCVisitor;
		ProjectVisitor  mProjectVisitor;
		StatisticUpdate mStatisticUpdate;
	public:
		Visitor(Workspace& _ws, std::string const& _name);
		void setCppVisitor(CppVisitor _cppVisitor) { mCppVisitor = _cppVisitor; }
		void setCVisitor(CVisitor _cVisitor) { mCVisitor = _cVisitor; }
		void setProjectVisitor(ProjectVisitor _projectVisitor) { mProjectVisitor = _projectVisitor; }
		void setStatisticUpdateCallback(StatisticUpdate _update) { mStatisticUpdate = _update; }

		void visit(int _threads);
	};
}
