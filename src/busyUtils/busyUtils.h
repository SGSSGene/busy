#pragma once

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
	void mv(std::string const& _src, std::string const& _dest);
	std::string dirname(std::string const& _file);
	std::string _basename(std::string const& _file);
	bool fileExists(std::string const& _file);
	bool dirExists(std::string const& _file);
//	std::string basename(std::string const& _file);

	auto listFiles(std::string const& _dir, bool recursive = false) -> std::vector<std::string>;
	auto listDirs(std::string const& _dir, bool _ignoreParent = false) -> std::vector<std::string>;

	void walkFiles(std::string _dir, std::function<void(std::string)> _func);

	bool isEndingWith(std::string const& str, std::string const& end);
	bool isStartingWith(std::string const& str, std::string const& start);

	auto explode(std::string const& _str, std::string const& _del) -> std::vector<std::string>;
	auto explode(std::string const& _str, std::vector<std::string> const& _del) -> std::vector<std::string>;

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

	bool validPackageName(std::string const& _str);


	class AtomicWrite {
		std::string mFileName;
		std::string mTempFileName;
	public:
		AtomicWrite(std::string  _fileName);
		void close();
		auto getTempName() const -> std::string const&;
	};

}
