#pragma once

#include <string>
#include <vector>

namespace commands {
	bool build(std::string const& rootProjectName, bool verbose, bool noconsole, int jobs);
	void clang();
	void clean();
	void cleanAll();
	void docu();
	void eclipse();
	void flavors(bool _isTerminal);
	void git(std::vector<std::string> const& _options);
	void test();
	void pull();
	void push();
	void info(std::vector<std::string> str);
	void status();
	void buildMode(std::string const& _buildMode);
	void quickFix();
	int  listFiles(std::string const& _relPath);
	void listProjects();
	void showDep(std::string const& rootProjectName);
	void toolchains(bool _isTerminal);
	void toolchain(std::string const& _toolchain);
}
