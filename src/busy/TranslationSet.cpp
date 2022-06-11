#include "TranslationSet.h"

namespace busy {

TranslationSet::TranslationSet(std::string _name, std::string _type, std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, LegacyIncludePaths _legacyIncludePaths, std::set<std::string> _systemLibraries)
    : mName               { std::move(_name) }
    , mType               { std::move(_type) }
    , mPath               { _sourcePath.lexically_normal() }
    , mLegacyIncludePaths { std::move(_legacyIncludePaths) }
    , mSystemLibraries    { std::move(_systemLibraries) }
{
    analyseFiles(_root, mPath, mLegacyIncludePaths);
}

void TranslationSet::analyseFiles(std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, LegacyIncludePaths const& _legacyIncludePaths) {
    // Discover header, cpp and c files
    if (is_directory(_root / _sourcePath)) {
        for (auto const &e : std::filesystem::recursive_directory_iterator(_root / _sourcePath)) {
            if (not is_regular_file(e)) {
                continue;
            }

            mFiles.emplace_back(_root, relative("/doesntexist/a/b/c/d/e" / e.path(), "/doesntexist/a/b/c/d/e" / _root).lexically_normal());
        }
    }

    // Discover legacy include paths
    for (auto const& [realDir, inclDir] : _legacyIncludePaths) {
        auto dir = realDir;
        if (realDir.is_relative()) {
            dir = _root / dir;
        }
        if (not is_directory(dir)) {
            continue;
        }
        for (auto const &e : std::filesystem::recursive_directory_iterator(dir)) {
            if (not is_regular_file(e)) {
                continue;
            }
            auto path = [&]() -> std::filesystem::path {
                if (realDir.is_relative()) {
                    return relative("/doesntexist/a/b/c/d/e" / e.path(), "/doesntexist/a/b/c/d/e" / _root).lexically_normal();
                }
                return e.path();
            }();
            mFiles.emplace_back(_root, std::move(path));
        }
    }
}

}
