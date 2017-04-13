#pragma once

#include <string>
#include <vector>

namespace commands {
	bool build(std::string const& rootProjectName, bool verbose, bool noconsole, int jobs, bool _dryRun, bool _strict);
	void clean();
	void cleanAll();
	void docu();
	void flavors(bool _isTerminal);
	void git(std::vector<std::string> const& _options);
	void test();
	void pull(int jobs);
	void push(int jobs);
	void info(std::vector<std::string> str);
	void status();
	void buildMode(std::string const& _buildMode);
	int  listFiles(std::string const& _relPath);
	void listProjects();
	void showDep(std::string const& rootProjectName);
	void toolchains(bool _isTerminal);
	void toolchain(std::string const& _toolchain);
}

namespace busy {
namespace commands {
	void genClangdb();
	void genEclipse();
}
}
