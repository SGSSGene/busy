#include <fmt/format.h>
#include <sargparse/sargparse.h>

#include "../FileCache.h"
#include "../cache.h"
#include "../utils.h"

namespace busy::cmd {
namespace {

void clean() {
    auto workPath   = std::filesystem::current_path();
    auto fileLock   = FileLock{};
    if (not exists(global_busyConfigFile)) {
        throw std::runtime_error("Can only be performed inside a build folder");
    }

    auto allRemovedFiles = std::uintmax_t{};
    for (auto& p : std::filesystem::directory_iterator{"."}) {
        auto path = p.path().lexically_normal();
        if (path != ".busy.yaml" and path != ".filecache" and path != ".lock") {
            allRemovedFiles += std::filesystem::remove_all(path);
        }
    }
    fmt::print("cleaned busy caches - removed {} files\n", allRemovedFiles);
}

auto cmd = sargp::Command{"clean", "cleans cache", clean};

}
}
