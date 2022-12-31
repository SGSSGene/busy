#pragma once

#include <filesystem>
#include <string>
#include <yaml-cpp/yaml.h>

namespace busy::answer {

struct Compilation {
    std::string stdout;
    std::string stderr;
    std::vector<std::string> dependencies;
    std::chrono::system_clock::time_point compileStartTime;
    double                                compileDuration;
    bool cached{};
    bool compilable{};
    std::vector<std::string> outputFiles{};
};

inline auto parseCompilation(std::string_view output) -> Compilation {
    try {
        auto node = YAML::Load(std::string{output});
        if (node.IsMap()) {

            auto dependencies = std::vector<std::string>{};
            if (node["dependencies"].IsSequence()) {
                dependencies = node["dependencies"].as<std::vector<std::string>>();
            }

            return Compilation {
                .stdout = node["stdout"].IsNull()?std::string{""}:node["stdout"].as<std::string>(""),
                .stderr = node["stderr"].IsNull()?std::string{""}:node["stderr"].as<std::string>(""),
                .dependencies = dependencies,
                .cached = node["cached"].as<bool>(),
                .compilable = node["compilable"].as<bool>(),
                .outputFiles = node["output_files"].as<std::vector<std::string>>(),
            };
        } else {
            return Compilation {
                .stdout = "",
                .stderr = "No valid return message",
            };
        }
    } catch (std::exception const& e) {
        return Compilation {
            .stdout = std::string{output},
            .stderr = e.what(),
        };
    }
}

}
