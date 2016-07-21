#include "busyUtils.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <libgen.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <process/Process.h>

namespace utils {
	void mkdir(std::string const& _dir) {
		std::stringstream call;
		process::Process p({"mkdir", "-p", _dir});
		if (p.getStatus() != 0) {
			throw std::runtime_error("error running mkdir -p " + _dir);
		}
	}
	void rm(std::string const& _dir, bool recursive, bool force ) {
		std::vector<std::string> call ({"rm"});
		if (recursive) {
			call.push_back("-r");
		}
		if (force) {
			call.push_back("-f");
		}
		call.push_back(_dir);
		process::Process p(call);
		if (p.getStatus() != 0) {
			throw std::runtime_error("error running rm");
		}
	}
	void mv(std::string const& _src, std::string const& _dest) {
		std::vector<std::string> call ({"mv", _src, _dest});
		process::Process p(call);
		if (p.getStatus() != 0) {
			throw std::runtime_error("error running rm");
		}
	}

	std::string dirname(std::string const& _file) {
		char c[_file.size()+1];
		memcpy(c, _file.c_str(), _file.size()+1);
		::dirname(c);
		return c;
	}
	std::string basename(std::string const& _file) {
		char c[_file.size()+1];
		memcpy(c, _file.c_str(), _file.size()+1);
		char *d = ::basename(c);
		return d;
	}

	std::string _basename(std::string const& _file) {
		return ::utils::basename(_file);
	}
	bool fileExists(std::string const& _file) {
		return std::ifstream(_file).good();
	}
	bool dirExists(std::string const& _file) {
		auto l = utils::explode(_file, "/");
		while (l.size() > 0 && l.back() == "") {
			l.pop_back();
		}
		std::string path;
		for (int i {0}; i < int(l.size()) -1; ++i) {
			path += l[i] + "/";
		}
		struct stat info;
		
		if(stat( path.c_str(), &info ) != 0) {
			return false;
		} else if(info.st_mode & S_IFDIR) {
			return true;
		}
		return false;
	}


	auto listFiles(std::string const& _dir, bool recursive) -> std::vector<std::string> {
		std::vector<std::string> entryList;

		DIR* dir;
		struct dirent* dirEntry;
		if((dir  = opendir(_dir.c_str())) == NULL) {
			throw std::runtime_error("Couldn't open dir to list files: " + _dir);
		}

		while ((dirEntry = readdir(dir)) != NULL) {
			if (dirEntry->d_type & DT_REG) {
				entryList.push_back(std::string(dirEntry->d_name));
			} else if (recursive && dirEntry->d_type & DT_DIR) {
				if (std::string(".") == dirEntry->d_name) continue;
				if (std::string("..") == dirEntry->d_name) continue;
				auto recList = listFiles(_dir + "/" + dirEntry->d_name, recursive);
				for (auto const& f : recList) {
					entryList.push_back(std::string(dirEntry->d_name) + "/" + f);
				}
			}
		}
		closedir(dir);
		std::sort(entryList.begin(), entryList.end());
		return entryList;
	}
	auto listDirs(std::string const& _dir, bool _ignoreParent) -> std::vector<std::string> {
		std::vector<std::string> entryList;

		DIR* dir;
		struct dirent* dirEntry;
		if((dir  = opendir(_dir.c_str())) == NULL) {
			throw std::runtime_error("Couldn't open dir: " + _dir);
		}


		while ((dirEntry = readdir(dir)) != NULL) {
			if (dirEntry->d_type & DT_DIR) {
				if (std::string(".") == dirEntry->d_name) continue;
				if (not _ignoreParent || std::string("..") != dirEntry->d_name) {
					entryList.push_back(std::string(dirEntry->d_name));
				}
			}
		}
		closedir(dir);
		std::sort(entryList.begin(), entryList.end());
		return entryList;
	}
	void impl_walkFiles(std::string const& _basedir, std::string const& _dir, std::function<void(std::string)> _func) {
		auto files = listFiles(_basedir+_dir);
		for (auto const& f : files) {
			_func(_dir + f);
		}
		auto dirs = listDirs(_basedir+_dir);
		for (auto const& d : dirs) {
			if (d != "." && d!= "..") {
				impl_walkFiles(_basedir, _dir + d + "/", _func);
			}
		}
	}

	void walkFiles(std::string _dir, std::function<void(std::string)> _func) {
		if (_dir.size() == 0) return;
		if (_dir.back() != '/') {
			_dir = _dir + '/';
		}
		impl_walkFiles(_dir, "", _func);
	}


	bool isEndingWith(std::string const& str, std::string const& end) {
		if (str.length() <= end.length()) {
			return false;
		}
		return strncmp(str.c_str() + str.length() - end.length(), end.c_str(), end.length()) == 0;
	}
	bool isStartingWith(std::string const& str, std::string const& start) {
		if (str.length() <= start.length()) {
			return false;
		}
		std::string sub = str.substr(0, start.length());

		return sub == start;
	}

	auto explode(std::string const& _str, std::string const& _del) -> std::vector<std::string> {
		auto str = _str;
		std::vector<std::string> retList;
		while (str.length() > 0) {
			auto p = str.find(_del);
			auto e = str.substr(0, p);
			if (e != "") {
				retList.emplace_back(std::move(e));
			}
			if (p == std::string::npos) {
				return retList;
			}
			str.erase(0, p + _del.size());
		}

		return retList;
	}

	auto explode(std::string const& _str, std::vector<std::string> const& _del) -> std::vector<std::string> {
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
			auto e = str.substr(0, p);
			if (e != "") {
				retList.emplace_back(std::move(e));
			}
			if (p == std::string::npos) {
				return retList;
			}
			str.erase(0, p + delSize);
		}

		return retList;
	}



	std::string runProcess(std::string const& _call) {
		FILE *fp = popen(_call.c_str(), "r");
		std::string returnString = "";
		char c;
		while (fgets(&c, 1, fp) && c != 0) {
			returnString += c;
		}
		fclose(fp);
		if (returnString.size() > 0) {
			returnString.pop_back();
		}
		return returnString;
	}

	int64_t getFileModificationTime(std::string const& _file) {
		if (not fileExists(_file)) return 0;
		struct stat buf;
		::stat(_file.c_str(), &buf);
		return buf.st_mtime;
	}
	std::string cwd() {
		char buf[512]; //!TODO risky
		char* ret = ::getcwd(buf, sizeof(buf));
		if (ret == nullptr) {
			throw std::runtime_error("getcwd faild");
		}
		return buf;
	}
	void cwd(std::string const& _string) {
		int ret = ::chdir(_string.c_str());
		if (ret == -1) {
			throw std::runtime_error("chdir to "+_string+" from "+cwd()+" failed");
		}
	}
	std::string sanitize(std::string const& _s) {
		std::string r;
		for (char c : _s) {
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				r += c;
			} else {
				r += "_";
			}
		}
		return r;
	}

	void sleep(unsigned int _s) {
		::sleep(_s);
	}

	AtomicWrite::AtomicWrite(std::string  _fileName)
		: mFileName (std::move(_fileName))
	{
		mTempFileName = mFileName + ".tempXXXXXX";
		std::vector<char> str(mTempFileName.size()+1);
		memcpy(str.data(), &mTempFileName.at(0), mTempFileName.size()+1);
		auto error = mkstemp(str.data());
		if (error == -1) {
			throw std::runtime_error(std::string("AtomicWrite doesn't work: ") + std::strerror(errno));
		}
		mTempFileName = str.data();




}
	void AtomicWrite::close() {
		int fd = open(mTempFileName.c_str(), O_APPEND);
		fsync(fd);
		::close(fd);
		rename(mTempFileName.c_str(), mFileName.c_str());
	}
	auto AtomicWrite::getTempName() const -> std::string const& {
		return mTempFileName;
	}
}

