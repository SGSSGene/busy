#pragma once

#include <iostream>
#include <filesystem>
#include <string>
#include <yaml-cpp/yaml.h>

namespace busy::desc {

struct TranslationSet {
    std::string              name;
    std::filesystem::path    path; // Not part of the busy.yaml file
    std::string              type;
    std::string              language;
    std::vector<std::string> dependencies;
    bool                     precompiled;
    struct {
        std::vector<std::tuple<std::string, std::string>> includes;
        std::vector<std::string> libraries;
    } legacy;
};

struct Desc {
    std::string                 version;
    std::filesystem::path       path;
    std::vector<TranslationSet> translationSets;
};

auto loadTupleList(YAML::Node node) {
    auto v = std::vector<std::tuple<std::string, std::string>>{};
    if (!node.IsDefined()) return v;
    if (!node.IsMap()) throw "wrong type";

    for (auto n : node) {
        v.emplace_back(n.first.as<std::string>(), n.second.as<std::string>());
    }
    return v;
}


auto loadTranslationSet(YAML::Node node, std::filesystem::path path, std::filesystem::path buildPath) {
    auto name = node["name"].as<std::string>();
    auto ts = TranslationSet {
        .name         = node["name"].as<std::string>(),
        .path         = path,
        .type         = node["type"].as<std::string>(),
        .language     = node["language"].as<std::string>(),
        .dependencies = node["dependencies"].as<std::vector<std::string>>(std::vector<std::string>{}),
        .precompiled  = node["precompiled"].as<bool>(false),
        .legacy {
            .includes  = loadTupleList(node["legacy"]["includes"]),
            .libraries = node["legacy"]["libraries"].as<std::vector<std::string>>(std::vector<std::string>{}),
        },
    };
    for (auto& [i1, i2] : ts.legacy.includes) {
        if (std::filesystem::path{i1}.is_absolute()) continue;
        std::cout << std::filesystem::current_path() << " " << i1 << " " << path << " " << std::filesystem::relative(i1, path) << " " << relative(path / i1) << "\n";
        i1 = relative(path / i1);
//        i1 = std::filesystem::relative(i1, path);

//        i1 = std::filesystem::relative(i1, path);
    }
    return ts;
}

auto loadTranslationSets(YAML::Node node, std::filesystem::path path, std::filesystem::path buildPath) {
    auto result = std::vector<TranslationSet>{};
    for (auto n : node) {
        result.emplace_back(loadTranslationSet(n, path, buildPath));
    }
    return result;
}

auto loadDesc(std::filesystem::path _file, std::filesystem::path _buildPath) -> Desc {
    auto root = YAML::LoadFile(_file);
    _file.remove_filename();
    auto path = _file / root["path"].as<std::string>(".");
    auto ret = Desc {
        .version         = root["version"].as<std::string>("0.0.1"),
        .path            = path,
        .translationSets = loadTranslationSets(root["translationSets"], path, _buildPath),
    };

    auto includes = root["include"];
    if (includes.IsSequence()) {
        for (auto include : includes) {
            auto path = _file / std::filesystem::path{include.as<std::string>()};
            for (auto d : std::filesystem::directory_iterator{path}) {
                if (!d.is_regular_file()) continue;
                auto desc = loadDesc(d.path(), _buildPath);
                for (auto ts : desc.translationSets) {
                    ret.translationSets.emplace_back(ts);
                }
            }
        }
    }
    return ret;
}

}
