#pragma once

#include "File.h"

#include <fmt/format.h>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <vector>


namespace busy {

class TranslationSet {
    using LegacyIncludePaths = std::vector<std::tuple<std::filesystem::path, std::filesystem::path>>;
private:
    std::string mName;
    std::string mType;
    std::filesystem::path mPath;

    std::vector<File>     mFiles;
    LegacyIncludePaths    mLegacyIncludePaths;
    std::set<std::string> mSystemLibraries;
    bool                  fullyDefined;


public:
    TranslationSet(std::string _name, std::string _type, std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, LegacyIncludePaths _legacyIncludePaths, std::set<std::string> _systemLibraries, bool _fullyDefined);

    [[nodiscard]]
    auto isFullyDefined() const {
        return fullyDefined;
    }

    [[nodiscard]]
    auto getFiles() const -> auto const&{
        return mFiles;
    }

    [[nodiscard]]
    auto getName() const -> std::string const& {
        return mName;
    }

    [[nodiscard]]
    auto getType() const -> std::string const& {
        return mType;
    }

    [[nodiscard]]
    auto getPath() const -> std::filesystem::path const& {
        return mPath;
    }

    [[nodiscard]]
    auto getSystemLibraries() const -> std::set<std::string> const& {
        return mSystemLibraries;
    }

    [[nodiscard]]
    auto getLegacyIncludePaths() const -> auto const& {
        return mLegacyIncludePaths;
    }

    [[nodiscard]]
    auto getIncludes() const -> std::set<std::filesystem::path> {
        auto ret = std::set<std::filesystem::path>{};
        for (auto const& f : getFiles()) {
            for (auto const& i : f.getIncludes()) {
                ret.emplace(i);
            }
        }
        return ret;
    }

    [[nodiscard]]
    auto isEquivalent(TranslationSet const& _other) const -> bool {
        if (mName != _other.mName) {
            return false;
        }
        if (mFiles.size() != _other.mFiles.size()) {
            return false;
        }
        if (mSystemLibraries != _other.mSystemLibraries) {
            return false;
        }


        auto hasSameFile = [&](auto const& _file) {
            for (auto const& f : _other.mFiles) {
                if (f.isEquivalent(_file)) {
                    return true;
                }
            }
            return false;
        };


        for (auto const& file : mFiles) {
            if (not hasSameFile(file)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]]
    auto difference(TranslationSet const& _other) const -> std::vector<std::string> {
        auto results = std::vector<std::string>{};
        if (mName != _other.mName) {
            results.emplace_back(fmt::format("different names \"{}\" → \"{}\"", mName, _other.mName));
        }
        if (mFiles.size() != _other.mFiles.size()) {
            results.emplace_back(fmt::format("different number of files {} → {}", mFiles.size(), _other.mFiles.size()));
        }
        if (mSystemLibraries != _other.mSystemLibraries) {
            auto appearLeft = std::vector<std::string>{};
            std::ranges::set_difference(mSystemLibraries, _other.mSystemLibraries, std::back_inserter(appearLeft));

            auto appearRight = std::vector<std::string>{};
            std::ranges::set_difference(_other.mSystemLibraries, mSystemLibraries, std::back_inserter(appearRight));

            auto s = fmt::format("different system libraries defined ({}) and ({})", fmt::join(appearLeft, ", "), fmt::join(appearRight, ", "));
            results.emplace_back(std::move(s));
        }

        auto hasSameFile = [&](auto const& _file) {
            for (auto const& f : _other.mFiles) {
                if (f.isEquivalent(_file)) {
                    return true;
                }
            }
            return false;
        };


        for (auto const& file : mFiles) {
            if (not hasSameFile(file)) {
                auto s = fmt::format("couldn't find the same file for {}", file.getPath().string());
                results.emplace_back(std::move(s));
            }
        }

        return results;
    }

private:
    void analyseFiles(std::filesystem::path const& _root, std::filesystem::path const& _sourcePath, LegacyIncludePaths const& _legacyIncludePaths);
};

}
