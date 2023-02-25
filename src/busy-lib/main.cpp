#include "Arguments.h"
#include "Desc.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"

#include <cassert>
#include <clice/clice.h>
#include <condition_variable>
#include <fmt/format.h>
#include <fmt/std.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

struct WorkQueue {
    struct Job {
        std::string              name;
        ssize_t                  blockingJobs{}; // Number of Jobs that are blocking this job
        std::function<void()>    job;
        std::vector<std::string> waitingJobs;    // Jobs that are waiting for this job
    };

    std::mutex                 mutex;
    std::condition_variable    cv;
    std::map<std::string, Job> allJobs;
    std::vector<std::string>   readyJobs;
    ssize_t                    jobsDone{};


    /* Inserts a job
     * \param name: name of this job a unique identifier
     * \param func: the function to execute to solve this job
     * \param blockingJobs: a list of jobs that have to be finished before this one (jobs that have to be executed before this one)
     */
    void insert(std::string name, std::function<void()> func, std::unordered_set<std::string> const& blockingJobs) {
        auto g = std::lock_guard{mutex};
        for (auto const& j : blockingJobs) {
            allJobs[j].waitingJobs.push_back(name);
        }
        auto& job = allJobs[name];
        if (!job.name.empty()) {
            throw std::runtime_error("duplicate job name \"" + name + "\", abort!");
        }
        job.name          = name;
        job.blockingJobs += ssize(blockingJobs);
        job.job           = std::move(func);

        if (blockingJobs.size() == 0) {
            readyJobs.emplace_back(name);
        }
    }

    bool processJob() {
        auto g = std::unique_lock{mutex};
        if (jobsDone == allJobs.size()) {
            return false;
        }

        if (!readyJobs.empty()) {
            auto last = readyJobs.back();
            readyJobs.pop_back();
            auto const& job = allJobs.at(last);
            g.unlock();
//            std::cout << "processing: " << job.name << "\n";
            job.job();
            finishJob(job.name);
        } else {
            cv.wait(g);
        }

        return true;
    }

private:
    void finishJob(std::string const& name) {
        auto g = std::lock_guard{mutex};
        for (auto j : allJobs.at(name).waitingJobs) {
            allJobs.at(j).blockingJobs -= 1;
            if (allJobs.at(j).blockingJobs == 0) {
                readyJobs.emplace_back(j);
            }
        }
        jobsDone += 1;
        cv.notify_all();
    }

public:
    void flush() {
        cv.notify_all();
    }
};


auto loadAllBusyFiles(Workspace& workspace, bool verbose) -> std::map<std::string, std::filesystem::path> {
    auto toolchains = std::map<std::string, std::filesystem::path>{};
    auto rootDir = workspace.busyFile;
    rootDir.remove_filename();
    // load other description files
    if (auto ptr = std::getenv("HOME")) {
        auto s = std::filesystem::path{ptr} / ".config/busy/env/share/busy";
        if (exists(s)) {
            for (auto const& d : std::filesystem::directory_iterator{s}) {
                if (!d.is_regular_file()) continue;
                auto desc = busy::desc::loadDesc(d.path(), rootDir, workspace.buildPath);
                for (auto ts : desc.translationSets) {
                    if (verbose) {
                        fmt::print("ts: {} (~/.config/busy/env/share/busy)\n", ts.name);
                    }
                    workspace.allSets[ts.name] = ts;
                    if (ts.type == "toolchain") {
                        auto path = absolute(d.path().parent_path().parent_path() / std::filesystem::path{ts.name} / "toolchain.sh");
                        toolchains[ts.name] = path;
                    }
                }
            }
        }
    }


    // load description as if "BUSY_ROOT" is the root, if non given, assuming "/usr"
    auto busy_root = [&]() -> std::filesystem::path {
        if (auto ptr = std::getenv("BUSY_ROOT")) {
            auto s = std::string{ptr};
            return s;
        }
        return "/usr";
    }();

    if (exists(busy_root / "share/busy")) {
        for (auto const& d : std::filesystem::directory_iterator{busy_root / "share/busy"}) {
            if (!d.is_regular_file()) continue;
            auto desc = busy::desc::loadDesc(d.path(), rootDir, workspace.buildPath);
            for (auto ts : desc.translationSets) {
                if (verbose) {
                    fmt::print("ts: {} (BUSY_ROOT)\n", ts.name);
                }
                workspace.allSets[ts.name] = ts;
                if (ts.type == "toolchain") {
                    auto path = absolute(d.path().parent_path().parent_path() / std::filesystem::path{ts.name} / "toolchain.sh");
                    toolchains[ts.name] = path;
                }
            }
        }
    }

    // load busyFile
    auto desc = busy::desc::loadDesc(workspace.busyFile, rootDir, workspace.buildPath);
    for (auto ts : desc.translationSets) {
        if (verbose) {
            fmt::print("ts: {} (local)\n", ts.name);
        }
        workspace.allSets[ts.name] = ts;
        if (ts.type == "toolchain") {
            auto path = absolute(std::filesystem::path{ts.name} / "toolchain.sh");
            toolchains[ts.name] = path;
        }
    }
    return toolchains;
}


int clice_main() {
//int main(int argc, char const* argv[]) {
    // ./build/bin/busy compile -f busy.yaml -t gcc12.2.sh

    // this will add cli options to the workspace
    auto updateWorkspace = [&](auto& workspace) {

        auto busyFile = [&]() -> std::optional<std::filesystem::path> {
            if (cliFile) return *cliFile;

            if (!workspace.firstLoad) return std::nullopt;

            for (auto p : {"busy.yaml", "../busy.yaml"}) {
                if (exists(std::filesystem::path{p})) {
                    return p;
                }
            }
        }();

        // set new busy file if set by commandline
        if (busyFile) {
            workspace.busyFile = *busyFile;
        }
    };

    auto updateWorkspaceToolchains = [&](auto& workspace, std::map<std::string, std::filesystem::path> const& toolchains) {
        // add more toolchains if set by commandline
        for (auto t : *cliToolchains) {
            if (toolchains.find(t) == toolchains.end()) {
                throw "unknown toolchain";
            }
            workspace.toolchains.emplace_back(*cliBuildPath, toolchains.at(t));
        }
    };

    try {
        auto otherNotSet = !cliModeStatus and !cliModeInfo and !cliModeInstall;
        if (cliModeCompile or otherNotSet) {
            auto workspace = Workspace{*cliBuildPath};
            updateWorkspace(workspace);

            auto toolchains = loadAllBusyFiles(workspace, cliVerbose);

            updateWorkspaceToolchains(workspace, toolchains);


            auto root = [&]() -> std::vector<std::string> {
                //!TODO how to do trailing values in clice?
                //if (args.trailing.size()) {
                //    return {args.trailing.front()};
                //}
                return workspace.findExecutables();
            }();

            auto wq = WorkQueue{};
            auto all = workspace.findDependencyNames(root); // All Translation units which root depence on
            for (auto r : root) {
                all.insert(r);
            }
            for (auto ts : all) {
                wq.insert(ts + "/setup", [ts, &workspace, &wq]() {
                    workspace._translateSetup(ts, cliVerbose);
                }, {});
                auto units = std::unordered_set<std::string>{};
                for (auto const& unit : workspace._listTranslateUnits(ts)) {
                    wq.insert(ts + "/unit/" + unit, [ts, &workspace, unit]() {
                        workspace._translateUnit(ts, unit, cliVerbose, cliClean, *cliOptions);
                    }, {ts + "/setup"});
                    units.emplace(ts + "/unit/" + unit);
                }
                units.emplace(ts + "/setup");
                for (auto dep : workspace.findDependencyNames(ts)) {
                    units.emplace(dep + "/linkage");
                }
                wq.insert(ts + "/linkage", [ts, &workspace]() {
                    workspace._translateLinkage(ts, cliVerbose, *cliOptions);
                }, units);
            }

            // translate all jobs
            std::atomic_bool errorAppeared{false};

            auto t = std::vector<std::jthread>{};
            for (ssize_t i{0}; i < *cliJobs; ++i) {
                t.emplace_back([&]() {
                    try {
                        while (!errorAppeared and wq.processJob());
                    } catch(std::exception const& e) {
                        if (!errorAppeared) {
                            fmt::print("compile error: {}\n", e.what());
                        }
                        errorAppeared = true;
                        wq.flush();
                    }
                });
            }
            t.clear();
            workspace.save();
            if (errorAppeared) {
                return 1;
            }
        } else if (cliModeStatus) {
            auto workspace = Workspace{*cliBuildPath};
            updateWorkspace(workspace);

            // load busy files
            auto toolchains = loadAllBusyFiles(workspace, cliVerbose);

            updateWorkspaceToolchains(workspace, toolchains);

            fmt::print("available ts:\n");
            for (auto type : {"executable", "library"}) {
                fmt::print("  {}:\n", type);
                for (auto const& [key, ts] : workspace.allSets) {
                    if (ts.type != type) continue;
                    fmt::print("    - {}{}{}\n", ts.name, ts.precompiled?" (precompiled)":"", ts.installed?" (installed)":"");
                }
            }
            fmt::print("available toolchains:\n");
            for (auto [key, value] : toolchains) {
                fmt::print("    {}: {}\n", key, value);
            }
            workspace.save();
        } else if (cliModeInfo) {
            auto workspace = Workspace{*cliBuildPath};
            updateWorkspace(workspace);

            auto rootDir = workspace.busyFile;
            rootDir.remove_filename();

            // load busyFile
            auto desc = busy::desc::loadDesc(workspace.busyFile, rootDir, workspace.buildPath);

            auto printTS = [&](std::string const& tsName) {
                fmt::print("{}:\n", tsName);
                auto const& ts = workspace.allSets.at(tsName);
                fmt::print("  type: {}\n", ts.type);
                fmt::print("  language: {}\n", ts.language);
                fmt::print("  precompiled: {}\n", ts.precompiled);
                fmt::print("  installed: {}\n", ts.installed);
                auto path = ts.path / "src" / ts.name;
                path = path.lexically_normal();
                fmt::print("  path: {}\n", path);
                fmt::print("  deps:\n");
                for (auto d : ts.dependencies) {
                    fmt::print("    - {}\n", d);
                }
            };
            for (auto ts : desc.translationSets) {
                workspace.allSets[ts.name] = ts;
                printTS(ts.name);
            }
            workspace.save();
        } else if (cliModeInstall) {
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
        }
    } catch (std::exception const& e) {
        fmt::print("{}\n", e.what());
    } catch (char const* p) {
        fmt::print("{}\n", p);
    }
    return 0;
}
