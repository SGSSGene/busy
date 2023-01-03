#pragma once

#include "Process.h"
#include "error_fmt.h"
#include "file_time.h"

#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <iostream>
#include <string>
#include <vector>

struct Toolchain {
    std::filesystem::path    buildPath;
    std::filesystem::path    toolchain;
    std::vector<std::string> languages;
    std::vector<std::string> options;

    Toolchain(std::filesystem::path _buildPath, std::filesystem::path _toolchain)
        : buildPath{std::move(_buildPath)}
        , toolchain{std::move(_toolchain)}
    {
        detectLanguages();
    }
private:
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

    auto formatCall(std::span<std::string> _cmd) const {
        if (buildPath == ".") {
            return fmt::format("{}", fmt::join(_cmd, " "));
        } else {
            return fmt::format("(cd {}; {})", buildPath.string(), fmt::join(_cmd, " "));
        }
    }

public:
    /** compiles a single translation unit
     */
    auto translateUnit(auto tuName, auto tuPath, bool verbose, std::vector<std::string> const& options) const {
        auto start = file_time.now();
        auto cmd = busy::genCall::compilation(toolchain, tuName, tuPath, options);

        auto call = formatCall(cmd);

        if (verbose) {
            fmt::print("{}\n", formatCall(cmd));
        }
        auto p = process::Process{cmd, buildPath};
        auto answer = busy::answer::parseCompilation(p.cout());
        if (!p.cerr().empty()) {
            throw error_fmt("Unexpected error with the build system: {}", p.cerr());
        }
        auto end = file_time.now();

        answer.compileStartTime = start;
        answer.compileDuration  = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.;

        return std::make_tuple(call, answer);
    }

    /**
     * setup translation set
     */
    void setupTranslationSet(auto const& ts, auto const& dependencies, bool verbose) const {
        auto cmd = busy::genCall::setup_translation_set(toolchain, buildPath, ts, dependencies);
        auto call = formatCall(cmd);
        if (verbose) {
            fmt::print("{}\n", formatCall(cmd));
        }
        auto p = process::Process{cmd, buildPath};
        if (verbose) {
            fmt::print("{}\n{}\n\n", p.cout(), p.cerr());
        }
    }

    /**
     * finish translation set
     */
    auto finishTranslationSet(auto const& ts, auto const& objFiles, auto const& dependencies, bool verbose) const {
        auto type = [&]() -> std::string {
            if (ts.type == "executable") {
                return "executable";
            } else if (ts.type == "library") {
                return "static_library";
            }
            throw "unknown translation set target";
        }();
        auto cmd = busy::genCall::linking(toolchain, ts, type, objFiles, dependencies);

        auto call = formatCall(cmd);

        if (verbose) {
            fmt::print("{}\n", formatCall(cmd));
        }
        auto p = process::Process{cmd, buildPath};
        if (verbose) {
            fmt::print("{}\n{}\n\n", p.cout(), p.cerr());
        }
        if (!p.cerr().empty()) {
            throw error_fmt("Unexpected error with the build system: {}", p.cerr());
        }
        auto answer = busy::answer::parseCompilation(p.cout());
        return std::make_tuple(call, answer);
    }
};

