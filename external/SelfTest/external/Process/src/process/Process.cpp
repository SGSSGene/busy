#include "Process.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1

namespace process {

static auto explode(std::string const& _str, std::vector<std::string> const& _del) -> std::vector<std::string> {
	auto str = _str;
	auto retList = std::vector<std::string>{};
	while (str.length() > 0) {
		auto p = str.find(_del[0]);
		int delSize = _del[0].length();
		for (auto const& s : _del) {
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


static auto fileExists(std::string const& _file) -> bool {
	return std::ifstream(_file).good();
}
static auto cwd() -> std::string {
	std::array<char, 512> buf{};
	char* ret = ::getcwd(buf.data(), buf.size());
	if (ret == nullptr) {
		throw std::runtime_error("getcwd faild");
	}
	return buf.data();
}
static void cwd(std::string const& _string) {
	int ret = ::chdir(_string.c_str());
	if (ret == -1) {
		throw std::runtime_error("chdir to "+_string+" from "+cwd()+" failed");
	}
}




class ProcessPImpl final {
	pid_t pid;
	std::array<int, 2> stdoutpipe;
	std::array<int, 2> stderrpipe;
	int status;

	std::string stdcout;
	std::string stdcerr;

	std::string oldCwd {"."};
public:
	ProcessPImpl(std::vector<std::string> const& prog, std::string const& _cwd) {
		int ret1 = pipe(stdoutpipe.data());
		int ret2 = pipe(stderrpipe.data());
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

	// NOLINTNEXTLINE(bugprone-exception-escape)
	~ProcessPImpl() {
		close(stdoutpipe[READ_END]);
		close(stderrpipe[READ_END]);
		cwd(oldCwd);
	}

	[[nodiscard]]
	auto cout() const -> std::string const& {
		return stdcout;
	}

	[[nodiscard]]
	auto cerr() const -> std::string const& {
		return stdcerr;
	}

	[[nodiscard]]
	auto getStatus() const -> int {
		return status;
	}
private:
	void childProcess(std::vector<std::string> const& _prog) {
		std::string envPath = getenv("PATH");
		auto s       = _prog[0];
		auto execStr = s;

		for (auto _s : explode(envPath, {":"})) {
			_s += "/";
			_s += s;
			if (fileExists(_s)) {
				execStr = _s;
				break;
			}
		}

		std::vector<char*> argv;
		argv.reserve(_prog.size());
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
				std::array<char, 4097> buffer;
				buffer[0] = 0;
				int size;
				if ((size = read(stdoutpipe[READ_END], buffer.data(), buffer.size() - 1)) >= 0) {
					buffer[size] = 0;
					_stdcout += buffer.data();
				} else return;
				if (size == 0) return;
			}
		}};
		std::thread t2 {[&] {
			while (true) {
				std::array<char, 4097> buffer;

				buffer[0] = 0;
				int size;
				if ((size = read(stderrpipe[READ_END], buffer.data(), buffer.size() - 1)) >= 0) {
					buffer[size] = 0;

					_stdcerr += buffer.data();
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
	: pimpl { std::make_unique<ProcessPImpl>(prog, cwd()) } {
}
Process::Process(std::vector<std::string> const& prog, std::string const& _cwd)
	: pimpl { std::make_unique<ProcessPImpl>(prog, _cwd) } {
}
Process::~Process() = default;

auto Process::cout() const -> std::string const& {
	return pimpl->cout();
}
auto Process::cerr() const -> std::string const& {
	return pimpl->cerr();
}
auto Process::getStatus() const -> int {
	return pimpl->getStatus();
}

}
