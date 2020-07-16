#include "FileCache.h"


#include <iostream>
#include <sys/stat.h>
#include <unordered_map>

namespace busy {

namespace {
template <typename Key, typename Value, typename RKey = Key>
struct Cache {
	using CB = std::function<Value(Key const&)>;
	CB cb;
	std::unordered_map<RKey, Value> cache{};

	Cache(CB _cb) : cb {std::move(_cb)} {}

	auto operator()(Key const& key) -> Value const& {
		auto iter = cache.find(RKey{key});
		if (iter == end(cache)) {
			auto value = cb(key);
			bool succ{};
			std::tie(iter, succ) = cache.try_emplace(RKey{key}, value);
		}
		return iter->second;
	}
};
}

auto computeHash(std::filesystem::path const& _file) -> std::string {
	static auto cache = Cache<std::filesystem::path, std::string, std::string>{
		[](std::filesystem::path const& _file) {
			std::ifstream ifs(_file, std::ios::binary);
			using Hash = std::array<char, picosha2::k_digest_size>;
			Hash hash;
			picosha2::hash256(ifs, hash.begin(), hash.end());
			//std::cout << "computing hash of: " << path << "\n";
			return base64::encode(std::string_view{&hash[0], hash.size()});
		}
	};
	return cache(_file);
}

auto getFileModificationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type {
	static auto cache = Cache<std::filesystem::path, std::filesystem::file_time_type, std::string>{
		[](std::filesystem::path const& _file) {
			/*struct stat buf;
			::stat(_file.c_str(), &buf);
			auto dur = std::filesystem::file_time_type::duration{buf.st_mtime};
			return std::filesystem::file_time_type{dur};*/
			return last_write_time(_file);
		}
	};
	return cache(_file);
}

auto getFileCreationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type {
	static auto cache = Cache<std::filesystem::path, std::filesystem::file_time_type, std::string>{
		[](std::filesystem::path const& _file) {
			struct stat buf;
			::stat(_file.c_str(), &buf);
			auto dur = std::filesystem::file_time_type::duration{buf.st_ctime};
			return std::filesystem::file_time_type{dur};
		}
	};
	return cache(_file);
}

auto getFileCache() -> FileCache& {
	static FileCache instance;
	return instance;
}

auto getFileData() -> FileData& {
	static FileData instance;
	return instance;
}

auto getFileInfos() -> FileInfos& {
	static FileInfos instance;
	return instance;
}



}
