#pragma once

#include <base64/base64.h>
#include <flattensObjectsNeatly/binary.h>
#include <flattensObjectsNeatly/chrono.h>
#include <flattensObjectsNeatly/filesystem.h>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>
#include <picosha2/picosha2.h>

namespace busy {

auto computeHash(std::filesystem::path const& path) -> std::string;
auto getFileModificationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type;
auto getFileCreationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type;

struct FileCache {
	using Path = std::filesystem::path;
	using Time = std::filesystem::file_time_type;
	using Hash = std::string;

	std::unordered_map<std::string, std::tuple<Time, Hash>> files;

	auto getHash(Path const& _path) -> std::string {
		auto str_path = std::string{_path};
		auto iter = files.find(str_path);
		if (iter == files.end()) {
			auto modTime = getFileModificationTime(_path);
			auto hash    = computeHash(_path);
			auto [succ, iter2] = files.try_emplace(str_path, modTime, hash);
			return hash;
		}
		if (std::get<Time>(iter->second) != getFileModificationTime(_path)) {
			auto modTime = getFileModificationTime(_path);
			auto hash    = computeHash(_path);
			iter->second = {modTime, hash};
			return hash;
		}
		return std::get<Hash>(iter->second);
	}

	template <typename Node>
	void serialize(Node& node) {
		node["files"]   % files;
	}
};

auto getFileCache() -> FileCache&;

struct FileData {
	using Path     = std::filesystem::path;
	using Includes = std::set<std::filesystem::path>;
	using Hash = std::string;

	template <typename Value>
	struct Date {
		Hash hash;
		Value value;

		template <typename Node>
		void serialize(Node& node) {
			node["hash"]  % hash;
			node["value"] % value;
		}
	};
	template <typename ...Args>
	using Data = std::tuple<Date<Args>...>;

	std::unordered_map<std::string, Data<Includes>> data;

	template <typename T, typename CB>
	auto checkAndRetrieve(Path const& _path, CB cb) -> T& {
		auto str_path = _path.string();
		auto hash     = getFileCache().getHash(str_path);
		auto iter     = data.find(str_path);
		if (iter != end(data)) {
			auto& date = std::get<Date<T>>(iter->second);
			if (date.hash == hash) {
				return date.value;
			}
		}
		auto& date = std::get<Date<T>>(data[str_path]);
		date.hash = hash;
		date.value = cb();
		return date.value;
	}

	template <typename Node>
	void serialize(Node& node) {
		node["data"] % data;
	}

};

auto getFileData() -> FileData&;

struct FileInfo {
	using Path         = std::filesystem::path;
	using Time         = std::filesystem::file_time_type;
	using Duration     = std::chrono::milliseconds;
	using Hash         = std::string;
	using Dependencies = std::vector<std::tuple<Path, Hash>>;

	Path path{};
	Time modTime{};
	Hash hash{};
	bool compilable{true};
	bool needRecompiling{true};
	Duration compileTime{0};
	Dependencies dependencies{};


	FileInfo(fon::ctor) {};
	FileInfo(Path _path)
		: path{_path}
	{}

	template <typename Node>
	void serialize(Node& node) {
		node["path"]            % path;
		node["modTime"]         % modTime;
		node["hash"]            % hash;
		node["compilable"]      % compilable;
		node["needRecompiling"] % needRecompiling;
		node["dependencies"]    % dependencies;
		node["compileTime"]     % compileTime;
	}


	bool hasChanged() const {
		if (needRecompiling) {
			return true;
		}
		if (modTime < getFileModificationTime(path) and getFileCache().getHash(path) != hash) {
			return true;
		}
		for (auto const& [d_path, d_hash] : dependencies) {
			if (modTime < getFileModificationTime(d_path) and getFileCache().getHash(d_path) != d_hash) {
				return true;
			}
		}
		return false;
	}

	void updateModTime(Time _modTime) {
		modTime = _modTime;
	}
	void updateHash(Hash const& _hash) {
		hash = _hash;
	}
	void updateDependencies(Dependencies const& _dependencies) {
		dependencies = _dependencies;
	}
	void updateCompilable(bool _compilable) {
		compilable = _compilable;
	}

	//!TODO remove when fixed?? otherwise not working with c++20
	void uselessCode() {
		FileInfo x{fon::ctor{}};
		FileInfo y {x};
	}
};

struct FileInfos {
	std::unordered_map<std::string, FileInfo> fileInfos;

	auto get(std::filesystem::path const& _path) -> FileInfo& {
		auto str_path = std::string{_path};
		auto iter = fileInfos.find(str_path);
		if (iter == end(fileInfos)) {
			auto [newIter, succ] = fileInfos.try_emplace(str_path, _path);
			iter = newIter;
		}
		return iter->second;
	}

	template <typename Node>
	void serialize(Node& node) {
		node["fileInfos"]   % fileInfos;
	}
};

auto getFileInfos() -> FileInfos&;



}
