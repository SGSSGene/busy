#pragma once

#include <cstdint>
#include <map>

namespace busy {

class FileStates {
private:
	std::map<std::string, int64_t> mFileModTime; // caching
	std::map<std::string, bool>    mFileChanged; // caching

public:
	auto getFileModTime(std::string const& s) -> int64_t;
	bool hasFileChanged(std::string const& s) const;
	void setFileChanged(std::string const& s, bool _changed);
};

inline FileStates& getFileStates() {
	static FileStates instance;
	return instance;
}
}
