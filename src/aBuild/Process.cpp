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
	ProcessPImpl(std::string const& s, std::vector<std::string> const& argv) {
		pipe(stdoutpipe);
		pipe(stderrpipe);

		pid=fork();
		if (pid==0) {
			childProcess(s, argv);
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
	void childProcess(std::string const& s, std::vector<std::string> const& _argv) {
		std::string execStr;
		std::string envPath = getenv("PATH");
		for (auto const& _s : utils::explode(envPath, ":")) {
			if (utils::fileExists(_s+"/"+s)) {
				execStr = _s+"/"+s;
				break;
			}
		}

		std::vector<char*> argv;
		argv.push_back(const_cast<char*>(s.c_str()));
		for (auto& a : _argv) {
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

		std::thread t {[&] {
			int i = 100;
			while (--i > 0) {
				char buffer[257];
				buffer[0] = 0;
				int size;
				if ((size = read(stdoutpipe[READ_END], buffer, sizeof(buffer)-1)) >= 0) {
					std::unique_lock<std::mutex> lock(mutex);
					buffer[size] = 0;
					_stdcout += buffer;
				} else return;
				buffer[0] = 0;
				if ((size = read(stderrpipe[READ_END], buffer, sizeof(buffer)-1)) >= 0) {
					std::unique_lock<std::mutex> lock(mutex);
					buffer[size] = 0;

					_stdcerr += buffer;
				} else return;
			}
		}};

		waitpid(pid, &status, 0); /* wait for child to exit */
		close(stdoutpipe[WRITE_END]);
		close(stderrpipe[WRITE_END]);
		t.join();
		{
			std::unique_lock<std::mutex> lock(mutex);
			stdcout = _stdcout;
			stdcerr = _stdcerr;
		}
	}
};

Process::Process(std::string const& s, std::vector<std::string> const& argv)
	: pimpl { new ProcessPImpl(s, argv) } {
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
