#include "Process.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <iostream>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>
#include "utils.h"

#define READ_END 0
#define WRITE_END 1

namespace utils {


class ProcessPImpl final {
	pid_t pid;
	int stdoutpipe[2];
	int stderrpipe[2];
	int status;

	std::string stdcout;
	std::string stdcerr;
public:
	ProcessPImpl(std::vector<std::string> const& prog) {
		int ret1 = pipe(stdoutpipe);
		int ret2 = pipe(stderrpipe);
		if (ret1 == -1 || ret2 == -1) {
			throw std::runtime_error("couldn't create pipes");
		}

		pid=fork();
		if (pid==0) {
			childProcess(prog);
		} else {
			parentProcess();
		}
	}

	~ProcessPImpl() {
		close(stdoutpipe[READ_END]);
		close(stderrpipe[READ_END]);
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

		for (auto const& _s : utils::explode(envPath, ":")) {
			if (utils::fileExists(_s+"/"+s)) {
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
		std::mutex mutex;
		std::string _stdcout, _stdcerr;

		std::thread t1 {[&] {
			while (true) {
				char buffer[4097];

				buffer[0] = 0;
				int size;
				if ((size = read(stdoutpipe[READ_END], buffer, sizeof(buffer)-1)) >= 0) {
					std::unique_lock<std::mutex> lock(mutex);
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
					std::unique_lock<std::mutex> lock(mutex);
					buffer[size] = 0;

					_stdcerr += buffer;
				} else return;
				if (size == 0) return;
			}
		}};


		waitpid(pid, &status, 0); /* wait for child to exit */
		close(stdoutpipe[WRITE_END]);
		close(stderrpipe[WRITE_END]);
		t1.join();
		t2.join();
		{
			std::unique_lock<std::mutex> lock(mutex);
			stdcout = _stdcout;
			stdcerr = _stdcerr;
		}
	}
};

Process::Process(std::vector<std::string> const& prog)
	: pimpl { new ProcessPImpl(prog) } {
}

Process::~Process() {
	delete pimpl;
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
