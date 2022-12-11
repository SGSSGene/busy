#include "Desc.h"
#include "genCall.h"
#include "answer.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"
#include "error_fmt.h"

#include <cassert>
#include <condition_variable>
#include <fmt/format.h>
#include <fmt/std.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

struct Arguments {
    std::string mode;
    std::filesystem::path buildPath{};
    std::optional<std::filesystem::path> busyFile;
    std::vector<std::filesystem::path> addToolchains;
    std::vector<std::string> trailing; // trailing commands
    std::vector<std::string> options; // toolchain options
    bool verbose{};
    bool clean{};
    std::filesystem::path prefix{}; // prefix for install

    Arguments(std::vector<std::string_view> args) {
        if (ssize(args) > 1) {
            mode = args[1];
        }
        for (int i{2}; i < ssize(args); ++i) {
            if (args[i] == "-f" and i+1 < ssize(args)) {
                ++i;
                busyFile = args[i];
            } else if (args[i] == "-t" and i+1 < ssize(args)) {
                ++i;
                addToolchains.emplace_back(args[i]);
            } else if (args[i] == "--options" and i+1 < ssize(args)) {
                ++i;
                options.emplace_back(args[i]);
            } else if (args[i] == "--clean") {
                clean = true;
            } else if (args[i] == "--verbose") {
                verbose = true;
            } else if (args[i] == "--prefix" and i+1 < ssize(args)) {
                ++i;
                prefix = args[i];
            } else {
                if (buildPath.empty()) {
                    buildPath = args[i];
                } else {
                    trailing.emplace_back(args[i]);
                }
            }
        }
        if (buildPath.empty()) {
            buildPath = ".";
        }
        if (options.empty()) {
            options.push_back("debug");
        }
    }
};

struct WorkQueue {
    struct Job {
        ssize_t                  blockingJobs{};
        std::function<void()>    job;
        std::vector<std::string> waitingJobs; // Jobs that are waiting for this job
    };

    std::mutex                 mutex;
    std::condition_variable    cv;
    std::map<std::string, Job> allJobs;
    std::vector<std::string>   readyJobs;
    ssize_t                    jobsDone{};


    /* Inserts a job
     * \param name: name of this job a unique identifier
     * \param func: the function to execute to solve this job
     * \param blockingJobs: a list of jobs that have to be finished before this one
     */
    void insert(std::string name, std::function<void()> func, std::unordered_set<std::string> const& blockingJobs) {
        auto job = Job {
            .blockingJobs = ssize(blockingJobs),
            .job          = [this, name, func]() {
                func();
                finishJobs(name);
            }
        };

        auto g = std::lock_guard{mutex};
        for (auto const& j : blockingJobs) {
            allJobs[j].waitingJobs.push_back(name);
        }
        allJobs[name].blockingJobs = job.blockingJobs;
        allJobs[name].job          = job.job;
        if (blockingJobs.size() == 0) {
            readyJobs.emplace_back(name);
        }
    }
    /* Insert a child job
     * Will check which other jobs it blocks and increase their count
     **/
    void insertChild(std::string name, std::string parentName, std::function<void()> func) {
         auto job = Job {
            .blockingJobs = 0,
            .job          = [this, name, func]() {
                func();
                finishJobs(name);
            },
            .waitingJobs = allJobs[parentName].waitingJobs
        };

        auto g = std::lock_guard{mutex};
        for (auto const& d :job.waitingJobs) {
            allJobs[d].blockingJobs += 1;
        }
        allJobs[name].job          = job.job;
        allJobs[name].waitingJobs  = job.waitingJobs;
        readyJobs.emplace_back(name);
    }

    bool processJob() {
        if (jobsDone == allJobs.size()) {
            return false;
        }

        auto g = std::unique_lock{mutex};
        if (!readyJobs.empty()) {
            auto last = readyJobs.back();
            readyJobs.pop_back();
            auto const& j = allJobs.at(last).job;
            g.unlock();
            j();
        } else {
            cv.wait(g);
        }

        return true;
    }

    void finishJobs(std::string const& name) {
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
                auto desc = busy::desc::loadDesc(d.path(), rootDir);
                for (auto ts : desc.translationSets) {
                    if (verbose) {
                        fmt::print("ts: {} (~/.config/busy/env/share/busy)\n", ts.name);
                    }
                    workspace.allSets[ts.name] = ts;
                }
                for (auto [key, value] : desc.toolchains) {
                    if (value.is_absolute()) {
                        toolchains[key] = value;
                    } else {
                        auto path = absolute(d);
                        path.remove_filename();
                        toolchains[key] = path / value;
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
            auto desc = busy::desc::loadDesc(d.path(), rootDir);
            for (auto ts : desc.translationSets) {
                if (verbose) {
                    fmt::print("ts: {} (BUSY_ROOT)\n", ts.name);
                }
                workspace.allSets[ts.name] = ts;
            }
            for (auto [key, value] : desc.toolchains) {
                if (value.is_absolute()) {
                    toolchains[key] = value;
                } else {
                    auto path = absolute(d);
                    path.remove_filename();
                    path = path / value;
                    toolchains[key] = path;
                }
            }
        }
    }

    // load busyFile
    auto desc = busy::desc::loadDesc(workspace.busyFile, rootDir);
    for (auto ts : desc.translationSets) {
        if (verbose) {
            fmt::print("ts: {} (local)\n", ts.name);
        }
        workspace.allSets[ts.name] = ts;
    }
    for (auto [key, value] : desc.toolchains) {
        if (value.is_absolute()) {
            toolchains[key] = value;
        } else {
            auto path = absolute(workspace.busyFile);
            path.remove_filename();
            toolchains[key] = relative(path / value, rootDir);
        }

    }
    return toolchains;
}


int main(int argc, char const* argv[]) {
    // ./build/bin/busy compile -f busy.yaml -t gcc12.2.sh
    auto args = [&]() {
        auto args = std::vector<std::string_view>{};
        args.reserve(argc);
        for (int i{0}; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return Arguments{args};
    }();


    // this will add cli options to the workspace
    auto updateWorkspace = [&](auto& workspace) {

        auto busyFile = args.busyFile;
        if (workspace.firstLoad and !busyFile) {
            if (exists(std::filesystem::path{"busy.yaml"})) {
                busyFile = "busy.yaml";
            } else if (exists(std::filesystem::path{"../busy.yaml"})) {
                busyFile = "../busy.yaml";
            }
        }

        // set new busy file if set by commandline
        if (busyFile) {
            workspace.busyFile = *busyFile;
        }
    };

    auto updateWorkspaceToolchains = [&](auto& workspace, std::map<std::string, std::filesystem::path> const& toolchains) {
        // add more toolchains if set by commandline
        for (auto t : args.addToolchains) {
            if (toolchains.find(t) == toolchains.end()) {
                throw "unknown toolchain";
            }
            workspace.toolchains.emplace_back(args.buildPath, toolchains.at(t));
        }
    };

    try {
        if (args.mode == "compile" || args.mode.empty()) {
            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            auto toolchains = loadAllBusyFiles(workspace, args.verbose);

            updateWorkspaceToolchains(workspace, toolchains);


            auto root = [&]() -> std::vector<std::string> {
                if (args.trailing.size()) {
                    return {args.trailing.front()};
                }
                return workspace.findExecutables();
            }();

            auto wq = WorkQueue{};
            auto all = workspace.findDependencyNames(root);
            for (auto r : root) {
                all.insert(r);
            }
            std::atomic_bool errorAppeared{false};
            for (auto ts : all) {
                auto deps = workspace.findDependencyNames(ts);
                wq.insert(ts, [ts, &workspace, &args, &wq]() {
                    for (auto [name, j] : workspace.translate(ts, args.verbose, args.clean, args.options)) {
                        wq.insertChild(name, ts, j);
                    }
                }, deps);
            }

            // translate all jobs
            auto t = std::vector<std::jthread>{};
            auto threadCt = 16;
            for (ssize_t i{0}; i < threadCt; ++i) {
                t.emplace_back([&]() {
                    try {
                        while (!errorAppeared and wq.processJob());
                    } catch(std::exception const& e) {
                        if (!errorAppeared) {
                            fmt::print("compile error: {}\n", e.what());
                            wq.flush();
                        }
                        errorAppeared = true;
                    }
                });
            }
            t.clear();
            workspace.save();
            if (errorAppeared) {
                return 1;
            }
        } else if (args.mode == "status") {
            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            // load busy files
            auto toolchains = loadAllBusyFiles(workspace, args.verbose);

            updateWorkspaceToolchains(workspace, toolchains);

            fmt::print("available ts:\n");
            for (auto type : {"executable", "library"}) {
                fmt::print("  {}:\n", type);
                for (auto const& [key, ts] : workspace.allSets) {
                    if (ts.type != type) continue;
                    fmt::print("    - {}{}\n", ts.name, ts.precompiled?" (precompiled)":"");
                }
            }
            fmt::print("available toolchains:\n");
            for (auto [key, value] : toolchains) {
                fmt::print("    {}: {}\n", key, value);
            }
            workspace.save();
        } else if (args.mode == "info") {
            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            // load busyFile
            auto desc = busy::desc::loadDesc(workspace.busyFile, workspace.buildPath);

            auto printTS = [&](std::string const& tsName) {
                fmt::print("{}:\n", tsName);
                auto const& ts = workspace.allSets.at(tsName);
                fmt::print("  type: {}\n", ts.type);
                fmt::print("  language: {}\n", ts.language);
                fmt::print("  precompiled: {}\n", ts.precompiled);
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
        } else if (args.mode == "install") {
            // Installs into local folder
            auto prefix = [&]() -> std::filesystem::path {
                if (!args.prefix.empty()) return args.prefix;
                if (auto ptr = std::getenv("HOME")) {
                    return std::filesystem::path{ptr} / ".config/busy/env";
                }
                throw error_fmt{"Trouble with the HOME variable, maybe it is not set?"};
            }();

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

            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            // load busyFile
            auto desc = busy::desc::loadDesc(workspace.busyFile, workspace.buildPath);

            for (auto ts : desc.translationSets) {
                if (ts.precompiled) continue;
                if (ts.type != "library") continue;
                if (ts.language == "c++") {
                    create_directories(prefix / "include");
                    create_directories(prefix / "share/busy");

                    // install includes
                    {
                        auto path = (ts.path / "src" / ts.name);
                        std::error_code ec;
                        auto options = std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive;
                        std::filesystem::copy(path, prefix / "include" / ts.name, options, ec);
                        if (ec) {
                            throw error_fmt{"could not copy {} to {} with message {}", path, prefix / "include" / ts.name, ec.message()};
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
                        ofs << "    precompiled: true\n";
                        ofs << "    legacy:\n";
                        if (hasLibrary.contains(ts.name + ".a")) {
                            ofs << "      libraries:\n";
                            ofs << "        - lib" << ts.name << "\n";
                        }
                        ofs << "      includes:\n";
                        ofs << "        ../../include/" << ts.name << ": " << ts.name << "\n";
                        ofs << "    dependencies:\n";
                        for (auto d : ts.dependencies) {
                            ofs << "      - " << d << "\n";
                        }

                    }
                } else {
                    throw error_fmt{"unknown install language {}", ts.language};
                }
            }
        } else {
            throw error_fmt{"unknown mode \"{}\", valid modes are \"compile\", \"status\", \"info\"", args.mode};
        }
    } catch (std::exception const& e) {
        fmt::print("{}\n", e.what());
    } catch (char const* p) {
        fmt::print("{}\n", p);
    }
}
