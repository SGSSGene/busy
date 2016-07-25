#pragma once

#include "utils.h"

namespace commands {

	using namespace busy;

	bool build(std::string const& rootProjectName, bool verbose = false, bool noconsole=false, int jobs = 10);
	void clang();
	void clean();
	void cleanAll();
	void docu();
	void eclipse();
	void git(std::vector<std::string> const& _options);
	void test();
	void pull();
	void push();
	void info(std::vector<std::string> str);
	void install();
	void status(std::string _buildMode = "");
	void quickFix();
	int  listFiles(std::string const& _relPath);
	void showDep(std::string const& rootProjectName);
	void toolchains();
	void toolchain(std::string const& _toolchain);
}
