#include "FileStates.h"

#include <busyUtils/busyUtils.h>

namespace busy {

auto FileStates::getFileModTime(std::string const& s) -> int64_t {
	auto iter = mFileModTime.find(s);
	if (iter != mFileModTime.end()) {
		return iter->second;
	}
	auto mod = utils::getFileModificationTime(s);
	mFileModTime[s] = mod;
	return mod;
}

bool FileStates::hasFileChanged(std::string const& s) const {
	auto iter = mFileChanged.find(s);
	if (iter == mFileChanged.end()) {
		return true;
	}
	return iter->second;
}
void FileStates::setFileChanged(std::string const& s, bool _changed) {
	mFileChanged[s] = _changed;
}


}
