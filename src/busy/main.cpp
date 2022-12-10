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
    bool verbose{};
    bool clean{};

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
            } else if (args[i] == "--clean") {
                clean = true;
            } else if (args[i] == "--verbose") {
                verbose = true;
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
        auto s = std::filesystem::path{ptr} / ".config/busy/packages";
        if (exists(s)) {
            for (auto const& d : std::filesystem::directory_iterator{s}) {
                if (!d.is_regular_file()) continue;
                auto desc = busy::desc::loadDesc(d.path(), rootDir);
                for (auto ts : desc.translationSets) {
                    if (verbose) {
                        fmt::print("ts: {} (~/.config/busy/packages)\n", ts.name);
                    }
                    workspace.allSets[ts.name] = ts;
                }
                for (auto [key, value] : desc.toolchains) {
                    if (value.is_absolute()) {
                        toolchains[key] = value;
                    } else {
                        auto path = absolute(d);
                        path.remove_filename();
                        toolchains[key] = relative(path / value, rootDir);
                    }
                }
            }
        }
    }


    // load other description files
    if (auto ptr = std::getenv("BUSY_PATH")) {
        auto s = std::string{ptr};
        for (auto const& d : std::filesystem::directory_iterator{s}) {
            if (!d.is_regular_file()) continue;
            auto desc = busy::desc::loadDesc(d.path(), rootDir);
            for (auto ts : desc.translationSets) {
                if (verbose) {
                    fmt::print("ts: {} (BUSY_PATH)\n", ts.name);
                }
                workspace.allSets[ts.name] = ts;
            }
            for (auto [key, value] : desc.toolchains) {
                if (value.is_absolute()) {
                    toolchains[key] = value;
                } else {
                    auto path = absolute(d);
                    path.remove_filename();
                    toolchains[key] = relative(path / value, rootDir);
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
    // ./build/bin/busy-desc compile build -f busy3.yaml -t toolchains.d/gcc12.1.sh
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
        if (args.mode == "compile") {
            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            auto toolchains = loadAllBusyFiles(workspace, args.verbose);

            updateWorkspaceToolchains(workspace, toolchains);


            auto root = [&]() -> std::string {
                if (args.trailing.size()) {
                    return args.trailing.front();
                }
                auto r = workspace.findExecutables();
                return r.front();
            }();

            auto wq = WorkQueue{};
            auto all = workspace.findDependencyNames(root);
            all.insert(root);
            std::atomic_bool errorAppeared{false};
            for (auto ts : all) {
                auto deps = workspace.findDependencyNames(ts);
                wq.insert(ts, [ts, &workspace, &args, &wq]() {
                    for (auto [name, j] : workspace.translate(ts, args.verbose, args.clean)) {
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
        } else if (args.mode == "status" || args.mode.empty()) {
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
        } else {
            throw error_fmt{"unknown mode \"{}\", valid modes are \"compile\", \"status\", \"info\"", args.mode};
        }
    } catch (std::exception const& e) {
        fmt::print("{}\n", e.what());
    } catch (char const* p) {
        fmt::print("{}\n", p);
    }
}
