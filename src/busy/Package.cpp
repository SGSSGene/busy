#include "Package.h"

#include <yaml-cpp/yaml.h>

namespace busy {

namespace fs = std::filesystem;

struct yaml_error : std::runtime_error {
    yaml_error(std::filesystem::path const& file, YAML::Node const& node)
        : runtime_error(file.string() + " in line " + std::to_string(node.Mark().line) + ":" + std::to_string(node.Mark().column) + " (" + std::to_string(node.Mark().pos) + ")")
    {}
    yaml_error(std::filesystem::path const& file, YAML::Node const& node, std::string const& msg)
        : runtime_error(file.string() + " in line " + std::to_string(node.Mark().line) + ":" + std::to_string(node.Mark().column) + " (" + std::to_string(node.Mark().pos) + ") - " + msg)
    {}

};


auto readPackage(std::filesystem::path _workspaceRoot, std::filesystem::path const& _file) -> Package {
    auto retTranslationSets = std::vector<TranslationSet>{};
    auto retPackages = std::vector<std::filesystem::path>{};

    auto _path = _file.parent_path();

    auto file = _file.filename();
    // read this package configuration
    auto path = _workspaceRoot / _path / file;


    try {
        auto node = YAML::LoadFile(path.string());
        retPackages.emplace_back(_workspaceRoot / _path);

        // check rootDir
        if (node["rootDir"].IsDefined()) {
            auto rel = std::filesystem::path{node["rootDir"].as<std::string>()};
            _path = (_path / rel).lexically_normal();
            _workspaceRoot = (_workspaceRoot / rel).lexically_normal();
        }

        // add all project names based on directory entries
        auto projectNames = std::set<std::string>{};
        auto sourcePath = _workspaceRoot / _path / "src";
        if (exists(sourcePath)) {
            for (auto& p : fs::directory_iterator(sourcePath)) {
                if (is_directory(p)) {
                    projectNames.insert(p.path().filename());
                }
            }
        }

        // scan all defined projects
        if (node["projects"].IsDefined()) {
            if (not node["projects"].IsSequence()) {
                throw yaml_error{path, node["projects"], "expected 'projects' as sequence"};
            }

            for (auto n : node["projects"]) {
                auto name = n["name"].as<std::string>();
                auto type = n["type"].as<std::string>("");
                auto path = _path / "src" / name;
                projectNames.erase(name);
                auto legacyIncludePaths    = std::vector<std::tuple<fs::path, fs::path>>{};
                auto legacySystemLibraries = std::set<std::string>{};

                for (auto const& e : n["legacy"]["includes"]) {
                    if (e.IsScalar()) {
                        auto path = std::filesystem::path{e.as<std::string>()};
                        if (path.is_relative()) {
                            path = _path / path;
                        }
                        legacyIncludePaths.emplace_back(path, name);
                    } else {
                        auto path1 = std::filesystem::path{begin(e)->first.as<std::string>()};
                        auto path2 = std::filesystem::path{begin(e)->second.as<std::string>()};
                        if (path1.is_relative()) path1 = _path / path1;
                        if (path2.is_relative()) path2 = _path / path2;
                        legacyIncludePaths.emplace_back(path1, path2);
                    }
                }
                for (auto const& e : n["legacy"]["systemLibraries"]) {
                    legacySystemLibraries.insert(e.as<std::string>());
                }
                retTranslationSets.emplace_back(name, type, _workspaceRoot, path, legacyIncludePaths, legacySystemLibraries);
            }
        }

        // add all projects that weren't defined in "projects" section of busy.yaml
        for (auto const& p : projectNames) {
            auto path = _path / "src" / p;
            retTranslationSets.emplace_back(p, "", _workspaceRoot, path, std::vector<std::tuple<fs::path, fs::path>>{}, std::set<std::string>{});
        }


        // iterate over all redefined external projects
        auto busyFileMaps = std::map<std::string, std::filesystem::path>{};
        if (node["external"].IsDefined()) {
            if (not node["external"].IsMap()) {
                throw yaml_error{path, node["external"], "expected 'external' as map"};
            }
            for (auto n : node["external"]) {
                auto key = n.first;
                auto value = n.second;
                busyFileMaps[key.as<std::string>()] = value.as<std::string>();
            }
        }


        // adding entries to package
        auto packages = std::vector<std::string>{};
        if (is_directory(_workspaceRoot / _path / external)) {
            for (auto& p : fs::directory_iterator(_workspaceRoot / _path / external)) {
                if (not is_symlink(p)) {
                    auto busyFile = [&]() -> std::filesystem::path {
                        auto externalPath = relative(p.path(), _workspaceRoot / _path);
                        if (busyFileMaps.count(externalPath.string()) > 0) {
                            return busyFileMaps.at(externalPath.string());
                        }
                        return relative(p.path() / "busy.yaml", _workspaceRoot);
                    }();
                    auto [projects, packages] = readPackage(_workspaceRoot, busyFile);
                    retTranslationSets.insert(end(retTranslationSets), begin(projects), end(projects));
                    retPackages.insert(end(retPackages), begin(packages), end(packages));
                }
            }
        }
    } catch(...) {
        std::throw_with_nested(std::runtime_error{"failed loading " + path.string()});
    }
    return {retTranslationSets, retPackages};
}


}
