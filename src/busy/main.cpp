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
    size_t                   jobs{1}; // how many threads/jobs in parallel?
    bool verbose{};
    bool clean{};
    std::filesystem::path prefix{}; // prefix for install

    Arguments(std::vector<std::string_view> args) {
        for (int i{1}; i < ssize(args); ++i) {
            if (args[i] == "-f" and i+1 < ssize(args)) {
                ++i;
                busyFile = args[i];
            } else if (args[i] == "-t" and i+1 < ssize(args)) {
                ++i;
                addToolchains.emplace_back(args[i]);
            } else if ((args[i] == "-j" or args[i] == "--jobs") and i+1 < ssize(args)) {
                ++i;
                jobs = std::stod(std::string{args[i]});
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
                if (args[i].size() and args[i][0] == '-') {
                    throw std::runtime_error("unknown argument " + std::string{args[i]});
                }
                if (mode.empty()) {
                    mode = args[i];
                } else if (buildPath.empty()) {
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
                auto desc = busy::desc::loadDesc(d.path(), rootDir);
                for (auto ts : desc.translationSets) {
                    if (verbose) {
                        fmt::print("ts: {} (~/.config/busy/env/share/busy)\n", ts.name);
                    }
                    workspace.allSets[ts.name] = ts;
                    if (ts.type == "toolchain") {
                        auto path = absolute(std::filesystem::path{ts.name} / "toolchain.sh");
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
            auto desc = busy::desc::loadDesc(d.path(), rootDir);
            for (auto ts : desc.translationSets) {
                if (verbose) {
                    fmt::print("ts: {} (BUSY_ROOT)\n", ts.name);
                }
                workspace.allSets[ts.name] = ts;
                if (ts.type == "toolchain") {
                    auto path = absolute(std::filesystem::path{ts.name} / "toolchain.sh");
                    toolchains[ts.name] = path;
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
        if (ts.type == "toolchain") {
            auto path = absolute(std::filesystem::path{ts.name} / "toolchain.sh");
            toolchains[ts.name] = path;
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
            auto all = workspace.findDependencyNames(root); // All Translation units which root depence on
            for (auto r : root) {
                all.insert(r);
            }
            for (auto ts : all) {
                wq.insert(ts + "/setup", [ts, &workspace, &args, &wq]() {
                    workspace._translateSetup(ts, args.verbose);
                }, {});
                auto units = std::unordered_set<std::string>{};
                for (auto const& unit : workspace._listTranslateUnits(ts)) {
                    wq.insert(ts + "/unit/" + unit, [ts, &workspace, &args, unit]() {
                        workspace._translateUnit(ts, unit, args.verbose, args.clean, args.options);
                    }, {ts + "/setup"});
                    units.emplace(ts + "/unit/" + unit);
                }
                units.emplace(ts + "/setup");
                for (auto dep : workspace.findDependencyNames(ts)) {
                    units.emplace(dep + "/linkage");
                }
                wq.insert(ts + "/linkage", [ts, &workspace, &args]() {
                    workspace._translateLinkage(ts, args.verbose);
                }, units);
            }

            // translate all jobs
            std::atomic_bool errorAppeared{false};

            auto t = std::vector<std::jthread>{};
            for (ssize_t i{0}; i < args.jobs; ++i) {
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
                    fmt::print("    - {}{}{}\n", ts.name, ts.precompiled?" (precompiled)":"", ts.installed?" (installed)":"");
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
        } else if (args.mode == "install") {
            // Installs into local folder
            auto prefix = [&]() -> std::filesystem::path {
                if (!args.prefix.empty()) return args.prefix;
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

            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            // load busyFile
            auto desc = busy::desc::loadDesc(workspace.busyFile, workspace.buildPath);

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
                        auto path = std::filesystem::path{key};
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
                        if (hasLibrary.contains(ts.name + ".a")) {
                            ofs << "      libraries:\n";
                            ofs << "        - " << ts.name << "\n";
                        }
                        ofs << "      includes:\n";
                        if (exists(ts.path / "src" / ts.name)) {
                            ofs << "        ../../include/" << ts.name << ": " << ts.name << "\n";
                        }
                        for (auto [key, value] : ts.legacy.includes) {
                            if (std::filesystem::path{key}.is_absolute()) {
                                ofs << "        " << key << ": " << value << "\n";
                            } else {
                                ofs << "        ../../include/" << ts.name << "/" << value << ": " << value << "\n";
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
        } else {
            throw error_fmt{"unknown mode \"{}\", valid modes are \"compile\", \"status\", \"info\"", args.mode};
        }
    } catch (std::exception const& e) {
        fmt::print("{}\n", e.what());
    } catch (char const* p) {
        fmt::print("{}\n", p);
    }
}
