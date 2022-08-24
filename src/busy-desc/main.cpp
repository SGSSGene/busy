#include "Desc.h"
#include "genCall.h"
#include "answer.h"
#include "Process.h"

#include <iostream>
#include <fmt/format.h>
#include <unordered_set>
#include <unordered_map>
#include <queue>

using TranslationMap = std::unordered_map<std::string, busy::desc::TranslationSet>;
struct Workspace {
    std::filesystem::path buildPath;
    std::filesystem::path toolchain;
    TranslationMap        allSets;
};

auto findDeps(busy::desc::TranslationSet const& root, TranslationMap const& map) {
    auto deps = std::vector<busy::desc::TranslationSet>{};
    auto found = std::unordered_set<std::string>{};

    auto open = std::queue<std::string>{};
    for (auto d : root.dependencies) {
        open.emplace(d);
    }
    found.insert(root.name);

    while (!open.empty()) {
        auto n = open.front();
        open.pop();
        found.insert(n);
        deps.push_back(map.at(n));
        for (auto d : map.at(n).dependencies) {
            open.emplace(d);
        }
    }

    return deps;
}

auto join(std::vector<std::string> c) -> std::string {
    if (c.empty()) return "";
    std::string s = c[0];
    for (size_t i{1}; i < c.size(); ++i) {
        s += " " + c[i];
    }
    return s;
}

auto translateUnit(auto tool, auto root, auto buildPath, auto tuPath) {
    auto cmd = busy::genCall::compilation(tool, root, tuPath, {"default"});

    auto call = std::string{fmt::format("(cd {}; {})", buildPath.string(), fmt::join(cmd, " "))};
    auto p = process::Process{cmd, buildPath};
    auto answer = busy::answer::parseCompilation(p.cout());
    if (!p.cerr().empty()) {
        throw std::runtime_error(std::string{"Unexpected error with the build system:"}
            + std::string{p.cerr()});
    }
    return std::make_tuple(call, answer);
}

void translate(busy::desc::TranslationSet const& target, Workspace const& workspace) {
    auto const& toolchain = workspace.toolchain;
    auto const& buildPath = workspace.buildPath;
    auto const& allSets   = workspace.allSets;
    auto deps = findDeps(target, allSets);
    for (auto d : deps) {
        translate(d, workspace);
    }
    std::cout << "\n\nTRANSLATING: " << target.name << "\n";
    if (target.precompiled) {
        return;
    }

    {
        auto cmd = busy::genCall::setup_translation_set(toolchain, buildPath, target, deps);
        std::cout << "(cd " << buildPath << "; " << join(cmd) << ")\n";
        auto p = process::Process{cmd, buildPath};
        auto o = p.cout();
        auto e = p.cerr();
        std::cout << o << "\n";
        std::cout << e << "\n";
        std::cout << "\n";
    }

    std::vector<std::filesystem::path> objFiles;
    auto tsPath = target.path / "src" / target.name;
    for (auto f : std::filesystem::recursive_directory_iterator(tsPath)) {
        if (f.is_regular_file()) {
            auto tuPath = relative(f, tsPath);
            objFiles.emplace_back(tuPath.string());
            auto [call, answer] = translateUnit(toolchain, target, buildPath, tuPath);
            std::cout << call << "\n";
            std::cout << answer.stdout << "\n";
            std::cout << "\n";
        }
    }
//    auto linking(std::filesystem::path const& _tool, std::string const& _type, std::filesystem::path output, std::vector<std::filesystem::path> const& _objFiles, desc::TranslationSet const& ts, std::vector<desc::TranslationSet> const& deps) {

    {
        auto type = [&]() -> std::string {
            if (target.type == "executable") {
                return "executable";
            } else if (target.type == "library") {
                return "static_library";
            }
            throw "unknown translation set target";
        }();
        auto cmd = busy::genCall::linking(toolchain, target, type, objFiles, deps);
        std::cout << "(cd " << buildPath << "; " << join(cmd) << ")\n";
        auto p = process::Process{cmd, buildPath};
        auto o = p.cout();
        auto e = p.cerr();
        std::cout << o << "\n";
        std::cout << e << "\n";

        std::cout << "\n";
    }
}

int main(int argc, char const* argv[]) {
    if (argc <= 1) return 1;
    // ./build/bin/busy-desc busy3.yaml build toolchains.d/gcc12.1.sh
    try {
        auto busyFile  = std::filesystem::path{argv[1]};
        auto workspace = Workspace {
            .buildPath = std::filesystem::path{argv[2]},
            .toolchain = std::filesystem::path{argv[3]},
        };
        workspace.toolchain = relative(absolute(workspace.toolchain), workspace.buildPath);

        std::string mainExe; //!TODO smarter way to decide this ;-)
        auto desc = busy::desc::loadDesc(busyFile, workspace.buildPath);
        for (auto ts : desc.translationSets) {
            if (mainExe.empty()) mainExe = ts.name;
            workspace.allSets[ts.name] = ts;
            auto path = desc.path / "src" / ts.name;
        }
        auto root = workspace.allSets.at(mainExe);
        translate(root, workspace);


    } catch (char const* p) {
        std::cout << p << "\n";
    }
}
