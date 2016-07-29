#pragma once

#include <functional>
#include <string>

namespace busy {
	class NeoWorkspace;
	class NeoProject;

	class NeoVisitor {
		using CppVisitor     = std::function<void(NeoProject const* _project, std::string const& _file)>;
		using CVisitor       = std::function<void(NeoProject const* _project, std::string const& _file)>;
		using ProjectVisitor = std::function<void(NeoProject const* _project)>;

		NeoWorkspace& mWorkspace;
		std::string   mTarget;

		CppVisitor     mCppVisitor;
		CVisitor       mCVisitor;
		ProjectVisitor mProjectVisitor;
	public:
		NeoVisitor(NeoWorkspace& _ws, std::string const& _name);
		void setCppVisitor(CppVisitor _cppVisitor) { mCppVisitor = _cppVisitor; }
		void setCVisitor(CVisitor _cVisitor) { mCVisitor = _cVisitor; }
		void setProjectVisitor(ProjectVisitor _projectVisitor) { mProjectVisitor = _projectVisitor; }
		void visit(int _threads);
	};
}
