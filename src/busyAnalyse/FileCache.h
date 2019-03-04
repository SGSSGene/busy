#pragma once

#include "utils/base64.h"

#include <picosha2/picosha2.h>

#include <flattensObjectsNeatly/flattensObjectsNeatly.h>
#include <flattensObjectsNeatly/chrono.h>
#include <flattensObjectsNeatly/filesystem.h>
#include <flattensObjectsNeatly/binary.h>

auto computeHash(std::filesystem::path const& path) -> std::string;
auto getFileModificationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type;
auto getFileCreationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type;


struct FileCache {

	using AllIncludes = std::tuple<std::set<std::filesystem::path>, std::set<std::filesystem::path>>;
	using Hash = std::string;

	template <typename T>
	struct Pair {
		std::filesystem::file_time_type modTime{};
		T                               value{};

		template <typename Node>
		void serialize(Node& node) {
			node["modTime"] % modTime;
			node["value"]   % value;
		}
	};


	template <typename ...Args>
	using PairTuple = std::tuple<Pair<Args>...>;

	struct Info {
		std::filesystem::file_time_type     cTime{};
		std::filesystem::file_time_type     modTime{};
		std::uintmax_t                      size{0};
		std::string                         hash{};

		PairTuple<std::vector<std::filesystem::path>, // dependency of projects(?)
		          AllIncludes,                        // includes
		          Hash                                // hash
		         > tuplePair;

		template <typename Node>
		void serialize(Node& node) {
			node["cTime"]      % cTime;
			node["modTime"]    % modTime;
			node["size"]       % size;
			node["hash"]       % hash;
			node["values"]     % tuplePair;
		}
	};

	std::map<std::string, Info> files;

	auto getFileCache(std::filesystem::path path) -> Info {
		auto iter = files.find(path);
		if (iter == files.end()) {
			updateFile(path);
		} else if (hasChanged(path)) {
			updateFile(path);
		} else {
		}
		return files.at(path);
	}



	void updateFile(std::filesystem::path path) {
		auto& info = files[path];
		info.cTime   = getFileCreationTime(path);
		info.modTime = last_write_time(path);
		info.size = 0;
		//info.size    = file_size(path);
		info.hash    = computeHash(path);
	}

	template <typename T>
	bool hasTChange(std::filesystem::path const& path) const {
		auto iter = files.find(path);
		if (iter == files.end()) {
			return true;
		}
		auto modTime = last_write_time(path);
		return modTime != std::get<Pair<T>>(iter->second.tuplePair).modTime;
	}

	template <typename T>
	void updateT(std::filesystem::path const& path, T value) {
		std::get<Pair<T>>(files[path].tuplePair).modTime = last_write_time(path);
		std::get<Pair<T>>(files[path].tuplePair).value   = std::move(value);
	}
	template <typename T>
	auto getT(std::filesystem::path const& path) -> T const& {
		return std::get<Pair<T>>(files.at(path).tuplePair).value;
	}


	bool hasChanged(std::filesystem::path const& path) const {
		auto iter = files.find(path);
		if (iter == files.end()) {
			return true;
		}

		auto modTime = last_write_time(path);
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
