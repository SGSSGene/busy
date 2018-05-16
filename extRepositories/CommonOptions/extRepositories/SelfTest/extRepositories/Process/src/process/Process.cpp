#include "Process.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>

#define READ_END 0
#define WRITE_END 1

namespace process {

static std::vector<std::string> explode(std::string const& _str, std::vector<std::string> const& _del) {
	auto str = _str;
	std::vector<std::string> retList;
	while (str.length() > 0) {
		auto p = str.find(_del[0]);
		int delSize = _del[0].length();
		for (auto const s : _del) {
			auto _p = str.find(s);
			if (_p == std::string::npos) continue;
			if (_p < p) {
				p = _p;
				delSize = s.length();
			}
		}
		auto _str = str.substr(0, p);
		retList.push_back(_str);
		if (p == std::string::npos) {
			return retList;
		}
		str.erase(0, p + delSize);
	}
	return retList;
}


static bool fileExists(std::string const& _file) {
	return std::ifstream(_file).good();
}
static std::string cwd() {
	char buf[512]; //!TODO risky
	char* ret = ::getcwd(buf, sizeof(buf));
	if (ret == nullptr) {
		throw std::runtime_error("getcwd faild");
	}
	return buf;
}
static void cwd(std::string const& _string) {
	int ret = ::chdir(_string.c_str());
	if (ret == -1) {
		throw std::runtime_error("chdir to "+_string+" from "+cwd()+" failed");
	}
}




class ProcessPImpl final {
	pid_t pid;
	int stdoutpipe[2];
	int stderrpipe[2];
	int status;

	std::string stdcout;
	std::string stdcerr;

	std::string oldCwd {"."};
public:
	ProcessPImpl(std::vector<std::string> const& prog, std::string _cwd) {
		int ret1 = pipe(stdoutpipe);
		int ret2 = pipe(stderrpipe);
		if (ret1 == -1 || ret2 == -1) {
			throw std::runtime_error("couldn't create pipes");
		}

		pid=fork();
		if (pid==0) {
			oldCwd = cwd();
			cwd(_cwd);
			childProcess(prog);
		} else {
			parentProcess();
		}
	}

	~ProcessPImpl() {
		close(stdoutpipe[READ_END]);
		close(stderrpipe[READ_END]);
		cwd(oldCwd);
	}
	auto cout() const -> std::string const& {
		return stdcout;
	}
	auto cerr() const -> std::string const& {
		return stdcerr;
	}
	int getStatus() const {
		return status;
	}
private:
	void childProcess(std::vector<std::string> const& _prog) {
		std::string envPath = getenv("PATH");
		auto s       = _prog[0];
		auto execStr = s;

		for (auto const& _s : explode(envPath, {":"})) {
			if (fileExists(_s+"/"+s)) {
				execStr = _s+"/"+s;
				break;
			}
		}

		std::vector<char*> argv;
		for (auto& a : _prog) {
			argv.push_back(const_cast<char*>(a.c_str()));
		}
		argv.push_back(nullptr);

		dup2(stdoutpipe[WRITE_END], STDOUT_FILENO);
		close(stdoutpipe[READ_END]);

		dup2(stderrpipe[WRITE_END], STDERR_FILENO);
		close(stderrpipe[READ_END]);

		execv(execStr.c_str(), &argv[0]);
		exit(127); /* only if execv fails */
	}
	void parentProcess() {
		std::string _stdcout, _stdcerr;

		std::thread t1 {[&] {
			while (true) {
				char buffer[4097];

				buffer[0] = 0;
				int size;
				if ((size = read(stdoutpipe[READ_END], buffer, sizeof(buffer)-1)) >= 0) {
					buffer[size] = 0;
					_stdcout += buffer;
				} else return;
				if (size == 0) return;
			}
		}};
		std::thread t2 {[&] {
			while (true) {
				char buffer[4097];

				buffer[0] = 0;
				int size;
				if ((size = read(stderrpipe[READ_END], buffer, sizeof(buffer)-1)) >= 0) {
					buffer[size] = 0;

					_stdcerr += buffer;
				} else return;
				if (size == 0) return;
			}
		}};


		waitpid(pid, &status, 0); /* wait for child to exit */
		status = status >> 8;
		close(stdoutpipe[WRITE_END]);
		close(stderrpipe[WRITE_END]);
		t1.join();
		t2.join();
		stdcout = _stdcout;
		stdcerr = _stdcerr;
	}
};

Process::Process(std::vector<std::string> const& prog)
	: pimpl { new ProcessPImpl(prog, cwd()) } {
}
Process::Process(std::vector<std::string> const& prog, std::string const& _cwd)
	: pimpl { new ProcessPImpl(prog, _cwd) } {
}



Process::~Process() {
}
auto Process::cout() const -> std::string const& {
	return pimpl->cout();
}
auto Process::cerr() const -> std::string const& {
	return pimpl->cerr();
}
int Process::getStatus() const {
	return pimpl->getStatus();
}

}
