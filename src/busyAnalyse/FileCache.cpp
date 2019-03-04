#include "FileCache.h"

#include <iostream>
#include <sys/stat.h>

auto computeHash(std::filesystem::path const& path) -> std::string {
	std::ifstream ifs(path, std::ios::binary);
	using Hash = std::array<unsigned char, picosha2::k_digest_size>;
	Hash hash;
	picosha2::hash256(ifs, hash.begin(), hash.end());
	//std::cout << "computing hash of: " << path << "\n";
	auto b64 = busy::base64_encode(&hash[0], picosha2::k_digest_size);
	return b64;
}


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
