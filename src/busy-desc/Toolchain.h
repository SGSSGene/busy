#pragma once

#include "Process.h"

#include <filesystem>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <iostream>

struct Toolchain {
    std::filesystem::path    buildPath;
    std::filesystem::path    toolchain;
    std::vector<std::string> languages;

    void detectLanguages() {
        auto cmd = std::vector<std::string>{toolchain, "info"};
        auto p = process::Process{cmd, buildPath};
        auto node = YAML::Load(std::string{p.cout()});
        for (auto n : node["toolchains"]) {
            for (auto l : n["languages"]) {
                languages.emplace_back(l.as<std::string>());
            }
        }
    }

    /** compiles a single translation unit
     */
    auto translateUnit(auto tuName, auto tuPath) const {
        auto cmd = busy::genCall::compilation(toolchain, tuName, tuPath, {"default"});

        auto call = std::string{fmt::format("(cd {}; {})", buildPath.string(), fmt::join(cmd, " "))};
        auto p = process::Process{cmd, buildPath};
        auto answer = busy::answer::parseCompilation(p.cout());
        if (!p.cerr().empty()) {
            throw std::runtime_error(std::string{"Unexpected error with the build system:"}
                + std::string{p.cerr()});
        }
        return std::make_tuple(call, answer);
    }

    /**
     * setup translation set
     */
    void setupTranslationSet(auto const& ts, auto const& dependencies) const {
        auto cmd = busy::genCall::setup_translation_set(toolchain, buildPath, ts, dependencies);
        fmt::print("(cd {}; {})\n", buildPath.string(), fmt::join(cmd, " "));
        auto p = process::Process{cmd, buildPath};
        auto o = p.cout();
        auto e = p.cerr();
        std::cout << o << "\n";
        std::cout << e << "\n";
        std::cout << "\n";
    }

    /**
     * finish translation set
     */
    void finishTranslationSet(auto const& ts, auto const& objFiles, auto const& dependencies) const {
        auto type = [&]() -> std::string {
            if (ts.type == "executable") {
                return "executable";
            } else if (ts.type == "library") {
                return "static_library";
            }
            throw "unknown translation set target";
        }();
        auto cmd = busy::genCall::linking(toolchain, ts, type, objFiles, dependencies);
        fmt::print("(cd {}; {})\n", buildPath.string(), fmt::join(cmd, " "));
        auto p = process::Process{cmd, buildPath};
        auto o = p.cout();
        auto e = p.cerr();
        std::cout << o << "\n";
        std::cout << e << "\n";
    }
};

