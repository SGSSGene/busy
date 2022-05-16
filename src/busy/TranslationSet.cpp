#include "TranslationSet.h"

namespace busy {

TranslationSet::TranslationSet(std::string _name, std::string _type, std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> _legacyIncludePaths, std::set<std::string> _systemLibraries)
    : mName               { std::move(_name) }
    , mType               { std::move(_type) }
    , mPath               { _sourcePath.lexically_normal() }
    , mLegacyIncludePaths { std::move(_legacyIncludePaths) }
    , mSystemLibraries    { std::move(_systemLibraries) }
{
    analyseFiles(_root, mPath, mLegacyIncludePaths);
}

void TranslationSet::analyseFiles(std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, std::vector<std::filesystem::path> const& _legacyIncludePaths) {
    // Discover header, cpp and c files
    for (auto const &e : std::filesystem::recursive_directory_iterator(_root / _sourcePath)) {
        if (not is_regular_file(e)) {
            continue;
        }

        mFiles.emplace_back(_root, relative("/doesntexist/a/b/c/d/e" / e.path(), "/doesntexist/a/b/c/d/e" / _root).lexically_normal());
    }

    // Discover legacy include paths
    for (auto const& dir : _legacyIncludePaths) {
        if (not is_directory(_root / dir)) {
            continue;
        }
        for (auto const &e : std::filesystem::recursive_directory_iterator(_root / dir)) {
            if (not is_regular_file(e)) {
                continue;
            }
            auto path = relative("/doesntexist/a/b/c/d/e" / e.path(), "/doesntexist/a/b/c/d/e" / _root).lexically_normal();
            mFiles.emplace_back(_root, std::move(path));
        }
    }
}

}
