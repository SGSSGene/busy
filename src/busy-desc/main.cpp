#include "Desc.h"
#include "genCall.h"
#include "answer.h"
#include "Process.h"

#include <iostream>
#include <fmt/format.h>
#include <unordered_set>
#include <unordered_map>
#include <queue>

struct Toolchain {
    std::filesystem::path buildPath;
    std::filesystem::path toolchain;

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

using TranslationMap = std::unordered_map<std::string, busy::desc::TranslationSet>;
struct Workspace {
    std::filesystem::path buildPath;
    TranslationMap        allSets;
    Toolchain             toolchain;

    /** Returns a list of TranslationSets that tu is depending on
     */
    auto findDependencies(busy::desc::TranslationSet const& tu) const {
        auto deps  = std::vector<busy::desc::TranslationSet>{};
        auto found = std::unordered_set<std::string>{};

        auto open = std::queue<std::string>{};
        for (auto d : tu.dependencies) {
            open.emplace(d);
        }
        found.insert(tu.name);

        while (!open.empty()) {
            auto n = open.front();
            open.pop();
            found.insert(n);
            deps.push_back(allSets.at(n));
            for (auto d : allSets.at(n).dependencies) {
                open.emplace(d);
            }
        }

        return deps;
    }

    /** Translates a tu and its dependencnies
     */
    void translate(busy::desc::TranslationSet const& ts) const {
        auto deps = findDependencies(ts);
        for (auto d : deps) {
            translate(d);
        }
        std::cout << "\n\nTRANSLATING: " << ts.name << "\n";
        if (ts.precompiled) {
            return;
        }

        toolchain.setupTranslationSet(ts, deps);

        auto objFiles = std::vector<std::filesystem::path>{};
        auto tsPath = ts.path / "src" / ts.name;
        for (auto f : std::filesystem::recursive_directory_iterator(tsPath)) {
            if (f.is_regular_file()) {
                auto tuPath = relative(f, tsPath);
                objFiles.emplace_back(tuPath.string());
                auto [call, answer] = toolchain.translateUnit(ts, tuPath);
                std::cout << call << "\n";
                std::cout << answer.stdout << "\n";
                std::cout << "\n";
            }
        }
        toolchain.finishTranslationSet(ts, objFiles, deps);
    }


};

int main(int argc, char const* argv[]) {
    if (argc <= 1) return 1;
    // ./build/bin/busy-desc busy3.yaml build toolchains.d/gcc12.1.sh
    try {
        auto busyFile  = std::filesystem::path{argv[1]};
        auto buildPath = std::filesystem::path{argv[2]};
        auto toolchain = std::filesystem::path{argv[3]};

        toolchain = relative(absolute(toolchain), buildPath);

        auto workspace = Workspace {
            .buildPath = buildPath,
            .toolchain = { .buildPath = buildPath,
                           .toolchain = toolchain },
        };

        std::string mainExe; //!TODO smarter way to decide this ;-)
        auto desc = busy::desc::loadDesc(busyFile, workspace.buildPath);
        for (auto ts : desc.translationSets) {
            if (mainExe.empty()) mainExe = ts.name;
            workspace.allSets[ts.name] = ts;
            auto path = desc.path / "src" / ts.name;
        }
        auto root = workspace.allSets.at(mainExe);
        workspace.translate(root);


    } catch (char const* p) {
        std::cout << p << "\n";
    }
}
