#ifndef EASYBUILD_UTILS_H
#define EASYBUILD_UTILS_H

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace utils {
	void mkdir(std::string const& _dir);
	void rm(std::string const& _dir, bool recursive=false, bool force=false);
	void resetFile(std::string const& _file, std::string const& _str);
	void cp(std::string const& _src, std::string const& _dest);
	std::string dirname(std::string const& _file);
	std::string _basename(std::string const& _file);
	bool fileExists(std::string const& _file);
//	std::string basename(std::string const& _file);

	std::vector<std::string> listFiles(std::string const& _dir, bool recursive = false);
	std::vector<std::string> listDirs(std::string const& _dir, bool _ignoreParent = false);

	void walkFiles(std::string _dir, std::function<void(std::string)> _func);

	bool isEndingWith(std::string const& str, std::string const& end);
	bool isStartingWith(std::string const& str, std::string const& start);

	std::vector<std::string> explode(std::string const& _str, std::string const& _del);

	std::string runProcess(std::string const& _call);

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
