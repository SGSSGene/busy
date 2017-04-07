#pragma once

#include <process/Process.h>
#include <stdlib.h>
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
	TestCase(std::string const& _name, std::vector<std::string> const& _params, int _status, std::string const& _stdcout, std::string const& _stdcerr)
		: name    { _name    }
		, params  { _params  }
		, status  { _status  }
		, stdcout { _stdcout }
		, stdcerr { _stdcerr } {
	}

	std::string const& getName() const {
		return name;
	}

	bool run(std::string const& _prog) {
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
	static SelfTest& getInstance() {
		static SelfTest instance;;
		return instance;
	}
	std::vector<TestCase> testCase;

	bool addCase(std::string _name, std::string _params, int _status, std::string _stdcout, std::string _stdcerr);

	bool runTests(std::string _prog) {
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

#define SELFTESTMAIN(argv) \
{ \
	char* c = getenv("selfTestRunning"); \
	if (c == nullptr || std::string(c) != "1") { \
		bool success = selfTest::SelfTest::getInstance().runTests(argv[0]); \
		if (not success) { \
			return -1; \
		} \
		return 0; \
	} \
}
#define SELFTEST(Name, Params, Status, Cout, Cerr) \
namespace { \
	auto var##Name = selfTest::SelfTest::getInstance().addCase(#Name, Params, Status, Cout, Cerr); \
} \

}

