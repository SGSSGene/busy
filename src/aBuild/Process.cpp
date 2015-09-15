#include "Process.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <iostream>
#include <string>
#include <sstream>

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
		std::vector<char*> argv;
		for (auto& a : _argv) {
			argv.push_back(const_cast<char*>(a.c_str()));
			std::cout<<a<<std::endl;
		}
		argv.push_back(nullptr);

		std::cout<<argv.size()<<std::endl;
		std::cout<<s<<" "<<argv[0]<<std::endl;

/*		dup2(stdoutpipe[WRITE_END], STDOUT_FILENO);
		close(stdoutpipe[READ_END]);

		dup2(stderrpipe[WRITE_END], STDERR_FILENO);
		close(stderrpipe[READ_END]);*/

		execv(s.c_str(), &argv[0]);
		std::cout<<"error"<<std::endl;
		exit(127); /* only if execv fails */
	}
	void parentProcess() {
		waitpid(pid, &status, 0); /* wait for child to exit */

		close(stdoutpipe[WRITE_END]);
		close(stderrpipe[WRITE_END]);

		char buffer[256];
		read(stdoutpipe[READ_END], buffer, sizeof(buffer));
		stdcout = buffer;

		read(stderrpipe[READ_END], buffer, sizeof(buffer));
		stdcerr = buffer;
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
