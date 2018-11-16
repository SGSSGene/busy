#pragma once

#include <picosha2/picosha2.h>

#include <flattensObjectsNeatly/flattensObjectsNeatly.h>
#include <flattensObjectsNeatly/chrono.h>
#include <flattensObjectsNeatly/filesystem.h>
#include <flattensObjectsNeatly/binary.h>

#include "base64.h"

#include <iostream>

inline auto computeHash(std::filesystem::path const& path) -> std::string {
	std::ifstream ifs(path, std::ios::binary);
	using Hash = std::array<unsigned char, picosha2::k_digest_size>;
	Hash hash;
	picosha2::hash256(ifs, hash.begin(), hash.end());
	std::cout << "computing hash of: " << path << "\n";
	auto b64 = base64_encode(&hash[0], picosha2::k_digest_size);
	return b64;
}

auto getFileModificationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type;
auto getFileCreationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type;




struct FileCache {
	struct Info {
		std::filesystem::file_time_type     cTime{};
		std::filesystem::file_time_type     modTime{};
		std::uintmax_t                      size{};
		std::string                         hash{};
		std::vector<std::filesystem::path>  dependent{};

		template <typename Node>
		void serialize(Node& node) {
			node["cTime"]      % cTime;
			node["modTime"]    % modTime;
			node["size"]       % size;
			node["hash"]       % hash;
			node["dependent"]  % dependent;
		}
	};

	std::map<std::string, Info> files;

	auto getFileCache(std::filesystem::path path) -> Info {
		auto iter = files.find(path);
		if (iter == files.end()) {
			updateFile(path, {});
		} else if (hasChanged(path)) {
			updateFile(path, {});
		} else {
		}
		return files.at(path);
	}



	void updateFile(std::filesystem::path path, std::vector<std::filesystem::path> dependent) {
		auto& info = files[path];
		info.cTime   = getFileCreationTime(path);
		info.modTime = last_write_time(path);
		info.size = 0;
		//info.size    = file_size(path);
		info.hash    = computeHash(path);

		info.dependent = std::move(dependent);
	}

	bool hasChanged(std::filesystem::path const& path) const {
		auto iter = files.find(path);
		if (iter == files.end()) {
			return true;
		}

		auto modTime = last_write_time(path);
		//auto modTime = getFileModificationTime(path);
		if (modTime != iter->second.modTime) {
			return true;
		}

		/*auto size = file_size(path);
		if (size != iter->second.size) {
			return true;
		}*/

/*		std::ifstream ifs(path, std::ios::binary);
		Hash hash;
		picosha2::hash256(ifs, hash.begin(), hash.end());

		if (hash != iter->second.hash) {
			return true;
		}*/

		/*for (auto const& path : iter->second.dependent) {
			if (hasChanged(path)) {
				return true;
			}
			
		}*/
		return false;
	}

	template <typename Node>
	void serialize(Node& node) {
		node["files"] % files;
	}
};

auto getFileCache() -> FileCache&;



