#pragma once

#include <filesystem>
#include <string>
#include <yaml-cpp/yaml.h>

namespace busy::answer {

struct Compilation {
    std::string stdout;
    std::string stderr;
    std::vector<std::string> dependencies;
    bool cached{};
    bool compilable{};
    std::vector<std::string> outputFiles{};
};

inline auto parseCompilation(std::string_view output) -> Compilation {
    try {
        auto node = YAML::Load(std::string{output});
        return Compilation {
            .stdout = node["stdout"].as<std::string>(""),
            .stderr = node["stderr"].as<std::string>(""),
            .dependencies = node["dependencies"].as<std::vector<std::string>>(),
            .cached = node["cached"].as<bool>(),
            .compilable = node["compilable"].as<bool>(),
            .outputFiles = node["output_files"].as<std::vector<std::string>>(),
        };
    } catch (std::exception const& e) {
        return Compilation {
            .stdout = std::string{output},
            .stderr = e.what(),
        };
    }
}

}
