#include "FileCache.h"

#include <sys/stat.h>

auto getFileModificationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type {
	struct stat buf;
	::stat(_file.c_str(), &buf);
	auto dur = std::filesystem::file_time_type::duration{buf.st_mtime};
	auto time = std::filesystem::file_time_type{dur};
	return time;
}

auto getFileCreationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type {
	struct stat buf;
	::stat(_file.c_str(), &buf);
	auto dur = std::filesystem::file_time_type::duration{buf.st_ctime};
	auto time = std::filesystem::file_time_type{dur};
	return time;
}




auto getFileCache() -> FileCache& {
	static FileCache instance;
	return instance;
}

