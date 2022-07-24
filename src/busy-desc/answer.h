#pragma once

#include <filesystem>
#include <string>
#include <yaml-cpp/yaml.h>

namespace busy::answer {

struct SetupTranslationUnit {};

struct Compilation {
    std::string stdout;
    std::string stderr;
    std::vector<std::string> dependencies;
    bool cached{};
    bool compilable{};
    std::vector<std::string> outputFiles{};
};

auto parseCompilation(std::string output) -> Compilation {
    auto node = YAML::Load(output);
    return Compilation {
        .stdout = node["stdout"].as<std::string>(),
        .stderr = node["stderr"].as<std::string>(),
        .dependencies = node["dependencies"].as<std::vector<std::string>>(),
        .cached = node["cached"].as<bool>(),
        .compilable = node["compilable"].as<bool>(),
        .outputFiles = node["output_files"].as<std::vector<std::string>>(),
    };
}

}
