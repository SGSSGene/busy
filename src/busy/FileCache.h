#pragma once

#include <base64/base64.h>
#include <fon/binary.h>
#include <fon/std/chrono.h>
#include <fon/std/filesystem.h>
#include <picosha2/picosha2.h>
#include <set>

namespace busy {

auto computeHash(std::filesystem::path const& path) -> std::string;
auto getFileModificationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type;
auto getFileCreationTime(std::filesystem::path const& _file) -> std::filesystem::file_time_type;

struct FileCache {
    using Path = std::filesystem::path;
    using Time = std::filesystem::file_time_type;
    using Hash = std::string;

    std::unordered_map<std::string, std::tuple<Time, Hash>> files;

    auto getHash(Path _path) -> std::string {
        _path = std::filesystem::relative(_path);
//        _path = _path.lexically_normal();
        auto str_path = std::string{_path};
        auto iter = files.find(str_path);
        if (iter == files.end()) {
            auto modTime = getFileModificationTime(_path);
            auto hash    = computeHash(_path);
            files.try_emplace(str_path, modTime, hash);
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

    template <typename Node, typename Self>
    static void reflect(Node& node, Self& self) {
        node["files"]   % self.files;
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

        template <typename Node, typename Self>
        static void reflect(Node& node, Self& self) {
            node["hash"]  % self.hash;
            node["value"] % self.value;
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

    template <typename Node, typename Self>
    static void reflect(Node& node, Self& self) {
        node["data"] % self.data;
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
    std::vector<Path> outputFiles{};
    Time modTime{};
    Hash hash{};
    bool compilable{true};
    bool needRecompiling{true};
    Duration compileTime{0};
    Dependencies dependencies{};
    bool hasWarnings{false};
    std::string compilerHash{};

    FileInfo() = default;

    FileInfo(fon::ctor) {};
    FileInfo(Path _path)
        : path{std::move(_path)}
    {}

    template <typename Node, typename Self>
    static void reflect(Node& node, Self& self) {
        node["path"]            % self.path;
        node["outputFiles"]     % self.outputFiles;
        node["modTime"]         % self.modTime;
        node["hash"]            % self.hash;
        node["compilable"]      % self.compilable;
        node["needRecompiling"] % self.needRecompiling;
        node["dependencies"]    % self.dependencies;
        node["compileTime"]     % self.compileTime;
        node["hasWarnings"]     % self.hasWarnings;
        node["compilerHash"]    % self.compilerHash;
    }


    [[nodiscard]]
    auto hasChanged() const -> bool {
        if (needRecompiling) {
            return true;
        }
        if (compilable) {
            for (auto const& outputFile : outputFiles) {
                if (not exists(outputFile)) {
                    return true;
                }
            }
        }
        // NOLINTNEXTLINE(modernize-use-nullptr)
        if (modTime < getFileModificationTime(path) and getFileCache().getHash(path) != hash) {
            return true;
        }
        for (auto const& [d_path, d_hash] : dependencies) {
            if (not exists(d_path)) {
                return true;
            }
            // NOLINTNEXTLINE(modernize-use-nullptr)
            if (modTime < getFileModificationTime(d_path) and getFileCache().getHash(d_path) != d_hash) {
                return true;
            }
        }
        return false;
    }

    //!TODO remove when fixed?? otherwise not working with c++20
    void uselessCode() {
        FileInfo x{fon::ctor{}};
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        FileInfo y {x};
    }
};

struct FileInfos {
    std::unordered_map<std::string, FileInfo> fileInfos;

    [[nodiscard]]
    auto get(std::filesystem::path const& _path) -> FileInfo& {
        auto str_path = std::string{_path};
        auto iter = fileInfos.find(str_path);
        if (iter == end(fileInfos)) {
            auto [newIter, succ] = fileInfos.try_emplace(str_path, _path);
            iter = newIter;
        }
        return iter->second;
    }

    template <typename Node, typename Self>
    static void reflect(Node& node, Self& self) {
        node["fileInfos"]   % self.fileInfos;
    }
};

auto getFileInfos() -> FileInfos&;



}
