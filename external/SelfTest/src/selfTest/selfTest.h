#pragma once

#include <process/Process.h>
#include <cstdlib>
#include <iostream>

namespace selfTest {

class TestCase {
private:
	std::string name;
	std::vector<std::string> params;
	int status;
	std::string stdcout;
	std::string stdcerr;
public:
	TestCase(std::string _name, std::vector<std::string> _params, int _status, std::string _stdcout, std::string _stdcerr)
		: name    { std::move(_name)    }
		, params  { std::move(_params)  }
		, status  { _status  }
		, stdcout { std::move(_stdcout) }
		, stdcerr { std::move(_stdcerr) } {
	}

	[[nodiscard]]
	auto getName() const -> std::string const& {
		return name;
	}

	auto run(std::string const& _prog) -> bool {
		params.insert(params.begin(), _prog);
		setenv("selfTestRunning", "1", 1);
		process::Process p { params };
		if (p.getStatus() != status) {
			return false;
		}
		if (p.cout() != stdcout) {
			return false;
		}

		if (p.cerr() != stdcerr) {
			return false;
		}
		return true;
	}
};

class SelfTest {
private:
public:
	static auto getInstance() -> SelfTest& {
		static SelfTest instance;;
		return instance;
	}
	std::vector<TestCase> testCase;

	auto addCase(std::string const& _name, std::string const& _params, int _status, std::string const& _stdcout, std::string const& _stdcerr) -> bool;

	auto runTests(std::string const& _prog) -> bool {
		bool success = true;
		for (auto& t : testCase) {
			if (not t.run(_prog)) {
				success = false;
				std::cout << "Test case: " << t.getName() << " failed" << std::endl;
			}
		}
		return success;
	}
};

#define SELFTESTMAIN(prog_name) \
{ \
	char* c = getenv("selfTestRunning"); \
	if (c == nullptr || std::string(c) != "1") { \
		bool success = selfTest::SelfTest::getInstance().runTests(prog_name); \
		if (not success) { \
			return EXIT_FAILURE; \
		} \
		return EXIT_SUCCESS; \
	} \
}
#define SELFTEST(Name, Params, Status, Cout, Cerr) \
namespace { \
	auto var##Name = selfTest::SelfTest::getInstance().addCase(#Name, Params, Status, Cout, Cerr); \
} \

}

