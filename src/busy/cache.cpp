#include "cache.h"

#include "FileCache.h"
#include "utils/utils.h"

#include <fon/std/filesystem.h>
#include <type_traits>

namespace busy {
namespace {
    bool emptyFileCache = {true};
}

using FileCacheStorage = std::tuple<FileCache, FileData, FileInfos>;
void loadFileCache() {
    std::tie(getFileCache(), getFileData(), getFileInfos()) = [&]() {
        // load binary cache
        if (std::filesystem::exists(".filecache")) {
            auto buffer = busy::utils::readFullFile(".filecache");
                emptyFileCache = false;
            return fon::binary::deserialize<FileCacheStorage>(buffer);
        // load yaml cache
        } else if (std::filesystem::exists(".filecache.yaml")) {
            emptyFileCache = false;
            return fon::yaml::deserialize<FileCacheStorage>(YAML::LoadFile(".filecache.yaml"));
        }
        return FileCacheStorage{};
    }();
}
void saveFileCache(bool _yamlCache) {
    if (emptyFileCache and getFileCache().files.empty() and getFileData().data.empty() and getFileInfos().fileInfos.empty()) {
        return;
    }

    auto tmp_data = getFileData();
    auto data = FileCacheStorage{getFileCache(), getFileData(), getFileInfos()};
    // write binary cache
    {
        auto node = fon::binary::serialize(data);
        auto ofs = std::ofstream{".filecache", std::ios::binary};
        ofs.write(reinterpret_cast<char const*>(node.data()), node.size());
    }

    // write yaml cache
    if (_yamlCache) {
        YAML::Emitter out;
        out << fon::yaml::serialize(data);
        std::ofstream(".filecache.yaml") << out.c_str();
    }
}

auto loadFileCache(bool _yamlCache) -> finally {
    loadFileCache();
    return finally{[_yamlCache]() {
        saveFileCache(_yamlCache);
    }};
}


}
