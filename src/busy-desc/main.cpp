#include "Desc.h"
#include "genCall.h"
#include "answer.h"
#include "Process.h"

#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <queue>

using TranslationMap = std::unordered_map<std::string, busy::desc::TranslationSet>;
TranslationMap allSets;

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

int main(int argc, char const* argv[]) {
    if (argc <= 1) return 1;
    try {
        auto buildPath = std::filesystem::path{"build"};
        auto path = std::filesystem::path{argv[1]};

        auto desc = busy::desc::loadDesc(path, buildPath);
        for (auto ts : desc.translationSets) {
            allSets[ts.name] = ts;
            auto path = desc.path / "src" / ts.name;
        }
        auto root = allSets.at("myApp");
        auto deps = findDeps(root, allSets);
        auto tool = std::string{"../../../toolchains.d/gcc12.1.sh"};

        {
            auto cmd = busy::genCall::setup_translation_set(tool, buildPath, root, deps);
            std::cout << "(cd " << buildPath << "; " << join(cmd) << ")\n";
            auto p = process::Process{cmd, buildPath};
            auto o = p.cout();
            auto e = p.cerr();
            std::cout << o << "\n";
            std::cout << e << "\n";
            std::cout << "\n";
        }


        std::vector<std::filesystem::path> objFiles;
        auto tsPath = root.path / "src" / root.name;
        for (auto f : std::filesystem::recursive_directory_iterator(tsPath)) {
            if (f.is_regular_file()) {
                std::cout << relative(f, tsPath) << "\n";
                auto cmd = busy::genCall::compilation(tool, root, relative(f, tsPath), {"default"});
                objFiles.emplace_back(relative(f, tsPath).string());

                std::cout << "(cd " << buildPath << "; " << join(cmd) << ")\n";
                auto p = process::Process{cmd, buildPath};
                auto o = p.cout();
                auto e = p.cerr();
                std::cout << o << "\n";
                std::cout << e << "\n";

                std::cout << "\n";
            }
        }
//    auto linking(std::filesystem::path const& _tool, std::string const& _type, std::filesystem::path output, std::vector<std::filesystem::path> const& _objFiles, desc::TranslationSet const& ts, std::vector<desc::TranslationSet> const& deps) {

        {
            auto cmd = busy::genCall::linking(tool, root, "executable", objFiles, deps);
            std::cout << "(cd " << buildPath << "; " << join(cmd) << ")\n";
            auto p = process::Process{cmd, buildPath};
            auto o = p.cout();
            auto e = p.cerr();
            std::cout << o << "\n";
            std::cout << e << "\n";

            std::cout << "\n";
        }




    } catch (char const* p) {
        std::cout << p << "\n";
    }
}
