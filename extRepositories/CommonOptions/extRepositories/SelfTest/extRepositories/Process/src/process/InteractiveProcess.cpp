#include "InteractiveProcess.h"

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <termios.h>
#include <unistd.h>


namespace process {

namespace utils {
	auto cwd() -> std::string {
		char buf[512]; //!TODO risky
		char* ret = ::getcwd(buf, sizeof(buf));
		if (ret == nullptr) {
			throw std::runtime_error("getcwd faild");
		}
		return buf;
	}
	void cwd(std::string const& _cwd) {
		int ret = ::chdir(_cwd.c_str());
		if (ret == -1) {
			throw std::runtime_error("chdir to "+_cwd+" from "+cwd()+" failed");
		}
	}

	auto explode(std::string const& _str, std::string const& _del) -> std::vector<std::string> {
		auto str = _str;
		std::vector<std::string> retList;
		while (str.length() > 0) {
			auto p = str.find(_del);
			retList.push_back(str.substr(0, p));
			if (p == std::string::npos) {
				return retList;
			}
			str.erase(0, p + _del.size());
		}

		return retList;
	}
}
namespace fileSystem {
	auto fileExists(std::string const& _file) -> bool {
		FILE* filePtr = fopen(_file.c_str(), "r");
		if (nullptr == filePtr)
			return (errno != ENOENT);

		fclose(filePtr);
		return true;
	}
}




class InteractiveProcessPImpl final {
	int   fdm, fds;
	int   rc;
	char  input[65536];
	pid_t pid;
	int   status;

	std::string oldCwd {"."};
public:
	InteractiveProcessPImpl(std::vector<std::string> const& prog, std::string const& _cwd) {
		fdm = posix_openpt(O_RDWR);
		if (fdm < 0) {
			throw std::runtime_error("InteractiveProcess: call posix_openpt() failed");
		}
		rc = grantpt(fdm);
		if (rc != 0) {
			throw std::runtime_error("InteractiveProcess: call grantpt() failed");
		}

		rc = unlockpt(fdm);
		if (rc != 0) {
			throw std::runtime_error("InteractiveProcess: call unlockpt() failed");
		}
		fds = open(ptsname(fdm), O_RDWR);

		pid = fork();
		if (pid == 0) {
			oldCwd = utils::cwd();
			utils::cwd(_cwd);

			childProcess(prog);
		} else {
			parentProcess();
		}
	}

	~InteractiveProcessPImpl() {
		utils::cwd(oldCwd);
	}
private:
	void childProcess(std::vector<std::string> const& _prog) {
		struct termios slave_orig_term_settings; // Saved terminal settings
		struct termios new_term_settings; // Current terminal settings

		// Close the master side of the PTY
		close(fdm);

		// Save the defaults parameters of the slave side of the PTY
		rc = tcgetattr(fds, &slave_orig_term_settings);

		// Set RAW mode on slave side of PTY
		new_term_settings = slave_orig_term_settings;
		cfmakeraw (&new_term_settings);
		tcsetattr (fds, TCSANOW, &new_term_settings);

		// The slave side of the PTY becomes the standard input and outputs of the child process
		close(0); // Close standard input (current terminal)
		close(1); // Close standard output (current terminal)
		close(2); // Close standard error (current terminal)

		int error;
		error = dup(fds); // PTY becomes standard input (0)
		if (error == -1) {
			throw std::runtime_error(std::string("Error in InteractiveProcess creating pipes: (") + std::to_string(errno) + ") " + strerror(errno));
		}

		error = dup(fds); // PTY becomes standard output (1)
		if (error == -1) {
			throw std::runtime_error(std::string("Error in InteractiveProcess creating pipes: (") + std::to_string(errno) + ") " + strerror(errno));
		}

		error = dup(fds); // PTY becomes standard error (2)
		if (error == -1) {
			throw std::runtime_error(std::string("Error in InteractiveProcess creating pipes: (") + std::to_string(errno) + ") " + strerror(errno));
		}


		// Now the original file descriptor is useless
		close(fds);

		// Make the current process a new session leader
		setsid();

		// As the child is a session leader, set the controlling terminal to be the slave side of the PTY
		// (Mandatory for programs like the shell to make them manage correctly their outputs)
		ioctl(0, TIOCSCTTY, 1);

		std::string envPath = getenv("PATH");
		auto s       = _prog[0];
		auto execStr = s;

		for (auto const& _s : utils::explode(envPath, {":"})) {
			if (fileSystem::fileExists(_s+"/"+s)) {
				execStr = _s+"/"+s;
				break;
			}
		}

		std::vector<char*> argv;
		for (auto& a : _prog) {
			argv.push_back(const_cast<char*>(a.c_str()));
		}
		argv.push_back(nullptr);

		execvp(execStr.c_str(), &argv[0]);
		exit(127); /* only if execv fails */
	}
	void parentProcess() {
		fd_set fd_in;

		// Close the slave side of the PTY
		close(fds);

		while (true) {
			// Wait for data from standard input and master side of PTY
			FD_ZERO(&fd_in);
			FD_SET(0, &fd_in);
			FD_SET(fdm, &fd_in);

			rc = select(fdm + 1, &fd_in, NULL, NULL, NULL);
			if (rc == -1) {
				throw std::runtime_error("InteractiveProcess: error on select()");
			}

			// If data on standard input
			if (FD_ISSET(0, &fd_in)) {
				rc = read(0, input, sizeof(input));
				if (rc <= 0) {
					throw std::runtime_error("error on reading standard input");
				}
				// Send data on the master side of PTY
				int error = write(fdm, input, rc);
				if (error == -1) {
					throw std::runtime_error(std::string("Error reading standard input: (") + std::to_string(errno) + ") " + strerror(errno));
				}
			}

			// If data on master side of PTY
			if (FD_ISSET(fdm, &fd_in)) {

				rc = read(fdm, input, sizeof(input));
				if (rc < 0) {
					waitpid(pid, &status, 0); /* wait for child to exit */
					return;
				}
				if (rc > 0) {
					// Send data on standard output
					int error = write(1, input, rc);
					if (error == -1) {
						throw std::runtime_error(std::string("Error reading standard input: (") + std::to_string(errno) + ") " + strerror(errno));
					}
				}
			}
		}
	}
};


InteractiveProcess::InteractiveProcess(std::vector<std::string> const& prog)
	: pimpl { new InteractiveProcessPImpl(prog, utils::cwd()) } {
}
InteractiveProcess::InteractiveProcess(std::vector<std::string> const& prog, std::string const& _cwd)
	: pimpl { new InteractiveProcessPImpl(prog, _cwd) } {
}

InteractiveProcess::~InteractiveProcess() {
}

}
