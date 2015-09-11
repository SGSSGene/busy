#ifndef EASYBUILD_UTILS_H
#define EASYBUILD_UTILS_H

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace utils {
	void mkdir(std::string const& _dir);
	void rm(std::string const& _dir, bool recursive=false, bool force=false);
	void resetFile(std::string const& _file, std::string const& _str);
	void cp(std::string const& _src, std::string const& _dest);
	void mv(std::string const& _src, std::string const& _dest);
	std::string dirname(std::string const& _file);
	std::string _basename(std::string const& _file);
	bool fileExists(std::string const& _file);
	bool dirExists(std::string const& _file);
//	std::string basename(std::string const& _file);

	std::vector<std::string> listFiles(std::string const& _dir, bool recursive = false);
	std::vector<std::string> listDirs(std::string const& _dir, bool _ignoreParent = false);

	void walkFiles(std::string _dir, std::function<void(std::string)> _func);

	bool isEndingWith(std::string const& str, std::string const& end);
	bool isStartingWith(std::string const& str, std::string const& start);

	std::vector<std::string> explode(std::string const& _str, std::string const& _del);

	std::string runProcess(std::string const& _call);

	template<typename T>
	void runParallel(std::vector<T> const& _args, std::function<void(T const& t)> _func, int batch = 4) {

		int batchCt = 0;
		while (batchCt < int(_args.size())) {
			std::vector<pid_t> pids;
			for (int i {batchCt}; i < batchCt + batch; ++i) {
				if (i >= int(_args.size())) break;
				auto const& a = _args[i];
				auto pid = fork();
				if (pid == 0) {
					_func(a);
					exit(0);
				}
				pids.push_back(pid);
			}
			for (auto const& p : pids) {
				while(0 < waitpid(p, nullptr, 0)) {}
			}
			batchCt += pids.size();
		}
	}

	int64_t getFileModificationTime(std::string const& _file);
	std::string cwd();
	void cwd(std::string const& _string);
	class Cwd {
		std::string oldPath;
	public:
		Cwd(std::string const& _path) {
			oldPath = cwd();
			cwd(_path);
		}
		~Cwd() {
			cwd(oldPath);
		}
	};

	std::string sanitize(std::string const& _s);

	void sleep(unsigned int _s);

}
#endif
