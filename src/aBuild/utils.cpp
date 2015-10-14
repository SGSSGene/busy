#include "utils.h"

#include <unistd.h>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iterator>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <libgen.h>

#include "Process.h"

namespace utils {
	void mkdir(std::string const& _dir) {
		std::stringstream call;
		Process p({"mkdir", "-p", _dir});
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
		Process p(call);
		if (p.getStatus() != 0) {
			throw std::runtime_error("error running rm");
		}
	}
	void mv(std::string const& _src, std::string const& _dest) {
		std::vector<std::string> call ({"mv", _src, _dest});
		Process p(call);
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
		std::string file = l[l.size()-1];
		for (auto const& s : listDirs(path, true)) {
			if (s == file) return true;
		}
		return false;
	}


	std::vector<std::string> listFiles(std::string const& _dir, bool recursive) {
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
		return entryList;
	}
	std::vector<std::string> listDirs(std::string const& _dir, bool _ignoreParent) {
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
		std::string sub = str.substr(str.length() - end.length());

		return sub == end;
	}
	bool isStartingWith(std::string const& str, std::string const& start) {
		if (str.length() <= start.length()) {
			return false;
		}
		std::string sub = str.substr(0, start.length());

		return sub == start;
	}

	std::vector<std::string> explode(std::string const& _str, std::string const& _del) {
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

	std::vector<std::string> explode(std::string const& _str, std::vector<std::string> const& _del) {
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


}

