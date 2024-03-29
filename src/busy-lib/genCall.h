#pragma once

#include "Desc.h"

#include <fmt/format.h>
#include <span>
#include <vector>

namespace busy::genCall {
inline auto setup_translation_set(std::filesystem::path const& _tool, std::filesystem::path _buildPath, desc::TranslationSet ts, std::span<desc::TranslationSet const> deps) {
    auto r = std::vector<std::string>{_tool.string(), "setup_translation_set", relative(ts.path, _buildPath).string(), ts.name};

    r.emplace_back("--ilocal");
    r.emplace_back("src/" + ts.name);

    r.emplace_back("--isystem");
    for (auto [key, value] : ts.legacy.includes) {
        r.emplace_back(fmt::format("\"{}:{}\"", key, value));
    }
    for (auto const& d : deps) {
        r.emplace_back(fmt::format("\"src/{}:{}\"", d.name, d.name));
        for (auto [key, value] : d.legacy.includes) {
            r.emplace_back(fmt::format("\"{}:{}\"", key , value));
        }
    }
    if (r.back() == "--isystem") r.pop_back();
    return r;
}
inline auto compilation(std::filesystem::path const& _tool, desc::TranslationSet const& ts, std::filesystem::path const& input, std::span<std::string const> options) {
    auto r = std::vector<std::string>{_tool.string(), "compile", ts.name};
    r.emplace_back(input.string());
    if (not options.empty()) {
        r.emplace_back("--options");
        for (auto const& o : options) {
            r.emplace_back(o);
        }
    }
    return r;
}
inline auto linking(std::filesystem::path const& _tool, desc::TranslationSet const& ts, std::string const& _type, std::span<std::filesystem::path const> _objFiles, std::span<desc::TranslationSet const> deps, std::span<std::string const> options) {
    auto r = std::vector<std::string>{_tool.string(), "link", ts.name, _type};

    if (not options.empty()) {
        r.emplace_back("--options");
        for (auto const& o : options) {
            r.emplace_back(o);
        }
    }

    r.emplace_back("--input");
    for (auto v : _objFiles) {
        r.emplace_back(v);
    }

    r.emplace_back("--llibraries");
    for (auto const& d : deps) {
        if (!d.precompiled && !d.installed) {
            r.emplace_back(d.name);
        }
    }
    if (r.back() == "--llibraries") r.pop_back();

    if (auto ptr = std::getenv("HOME")) {
        r.emplace_back("--syslibrarypaths");
        auto s = std::filesystem::path{ptr} / ".config/busy/env/lib";
        r.emplace_back(s);
    }

    r.emplace_back("--syslibraries");
    for (auto v : ts.legacy.libraries) {
        r.emplace_back(v);
    }
    for (auto const& d : deps) {
        for (auto v : d.legacy.libraries) {
            r.emplace_back(v);
        }
    }
    if (r.back() == "--syslibraries") r.pop_back();
    return r;
}
}
