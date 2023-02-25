#include "Arguments.h"
#include "Desc.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"
#include "utils.h"


namespace {
auto _ = cliModeInstall.run([]() {
    // Installs into local folder
    auto prefix = [&]() -> std::filesystem::path {
        if (cliPrefix) return *cliPrefix;
        if (auto ptr = std::getenv("HOME")) {
            return std::filesystem::path{ptr} / ".config/busy/env";
        }
        throw error_fmt{"Trouble with the HOME variable, maybe it is not set?"};
    }();

    // copy all binaries to target folder
    if (auto p = std::filesystem::path{"bin"}; is_directory(p)) {
        create_directories(prefix / p);
        for (auto const& d : std::filesystem::directory_iterator{p}) {
            std::error_code ec;
            std::filesystem::copy(d.path(), prefix / p, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                throw error_fmt{"could not copy {} to {} with message {}", absolute(d.path()), prefix / p, ec.message()};
            }
        }
    }
    // copy libraries to target folder
    auto hasLibrary = std::unordered_set<std::string>{};
    if (auto p = std::filesystem::path{"lib"}; is_directory(p)) {
        create_directories(prefix / p);
        for (auto const& d : std::filesystem::directory_iterator{p}) {
            auto tsName = d.path().filename().string();
            hasLibrary.insert(tsName);
            auto targetPath = prefix / p / ("lib" + tsName);
            std::error_code ec;
            std::filesystem::copy(d.path(), targetPath, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                throw error_fmt{"could not copy {} to {} with message {}", absolute(d.path()), targetPath, ec.message()};
            }
        }
    }

    auto workspace = Workspace{*cliBuildPath};
    updateWorkspace(workspace);

    auto rootDir = workspace.busyFile;
    rootDir.remove_filename();

    // load busyFile
    auto desc = busy::desc::loadDesc(workspace.busyFile, rootDir, workspace.buildPath);

    for (auto ts : desc.translationSets) {
        if (ts.installed) continue;
        if (ts.language == "c++") {
            if (ts.type != "library") continue;
            create_directories(prefix / "include");
            create_directories(prefix / "share/busy");

            // remove all previous installed files
            std::filesystem::remove_all(prefix / "include" / ts.name);

            // install includes
            {
                auto path = ts.path / "src" / ts.name;
                // any files to copy?
                if (exists(path)) {
                    std::error_code ec;
                    auto options = std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive | std::filesystem::copy_options::copy_symlinks;
                    std::filesystem::copy(path, prefix / "include" / ts.name, options, ec);
                    if (ec) {
                        throw error_fmt{"could not copy {} to {} with message {}", path, prefix / "include" / ts.name, ec.message()};
                    }
                }
            }
            // copy legacy includes that are relative path over
            for (auto [key, value] : ts.legacy.includes) {
                auto rootPath = workspace.busyFile;
                rootPath.remove_filename();
                auto path = std::filesystem::path{key};
                if (path.is_relative()) {
                    path = rootPath / path;
                }
                if (path.is_relative()) {
                    std::error_code ec;
                    auto options = std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive | std::filesystem::copy_options::copy_symlinks;
                    auto target = prefix / "include" / ts.name / value;
                    std::filesystem::copy(path, target, options, ec);
                    if (ec) {
                        throw error_fmt{"could not copy {} to {} with message {}", path, target, ec.message()};
                    }
                }
            }


            // install a busy.yaml file
            {
                auto path = prefix / "share/busy" / ts.name;
                path.replace_extension("yaml");
                auto ofs = std::ofstream{path};
                ofs << "file-version: 1.0.0\n";
                ofs << "translationSets:\n";
                ofs << "  - name: " << ts.name << "\n";
                ofs << "    type: library\n";
                ofs << "    language: c++\n";
                ofs << "    installed: true\n";
                ofs << "    legacy:\n";
                ofs << "      libraries:\n";
                if (hasLibrary.contains(ts.name + ".a")) {
                    ofs << "        - \"" << ts.name << "\"\n";
                }
                for (auto l : ts.legacy.libraries) {
                    ofs << "        - \"" << l << "\"\n";
                }
                ofs << "      includes:\n";
                if (exists(ts.path / "src" / ts.name)) {
                    ofs << "        ../../include/" << ts.name << ": \"" << ts.name << "\"\n";
                }
                for (auto [key, value] : ts.legacy.includes) {
                    if (std::filesystem::path{key}.is_absolute()) {
                        ofs << "        " << key << ": \"" << value << "\"\n";
                    } else {
                        ofs << "        ../../include/" << ts.name << "/" << value << ": \"" << value << "\"\n";
                    }
                }
                if (ts.dependencies.size()) {
                    ofs << "    dependencies:\n";
                    for (auto d : ts.dependencies) {
                        ofs << "      - " << d << "\n";
                    }
                }

            }
        } else if (ts.type == "toolchain") {
            create_directories(prefix / "share/busy");
            create_directories(prefix / "include");
            auto path = prefix / "share/busy" / (ts.name + ".yaml");
            auto ofs = std::ofstream{path};
            ofs << "file-version: 1.0.0\n";
            ofs << "translationSets:\n";
            ofs << "  - name: " << ts.name << "\n";
            ofs << "    type: toolchain\n";

            // install includes
            {
                auto path = ts.path / "src" / ts.name;
                std::error_code ec;
                auto options = std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive | std::filesystem::copy_options::copy_symlinks;
                std::filesystem::copy(path, prefix / "share" / ts.name, options, ec);
                if (ec) {
                    throw error_fmt{"could not copy {} to {} with message {}", path, prefix / "share" / ts.name, ec.message()};
                }
            }
        } else {
            throw error_fmt{"unknown install language {}", ts.language};
        }
    }

    exit(0);
});

}
