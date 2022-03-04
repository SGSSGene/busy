#include "utils.h"

#include "toolchains.h"
#include "overloaded.h"

#include <algorithm>
#include <cstdio>
#include <fcntl.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fon/std/all.h>
#include <fon/yaml.h>
#include <fstream>
#include <iostream>
#include <process/Process.h>
#include <queue>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>


namespace busy {

FileLock::FileLock() {
    fd = ::open(global_lockFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        throw std::runtime_error("can't access .lock");
    }
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        throw std::runtime_error("other process is running");
    }
}
FileLock::~FileLock() {
    close(fd);
    remove(global_lockFile);
}


void printTranslationSets(std::map<TranslationSet const*, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>>> const& _projects) {
    for (auto const& [i_project, dep] : _projects) {
        auto const& project = *i_project;
        auto const& dependencies = std::get<0>(dep);
        fmt::print("\n");

        fmt::print("  - project-name: {}\n", project.getName());
        fmt::print("    path: {}\n", project.getPath());
        if (not project.getSystemLibraries().empty()) {
            fmt::print("    systemLibraries:\n");
            for (auto const& l : project.getSystemLibraries()) {
                fmt::print("    - {}\n", l);
            }
        }
        if (not dependencies.empty()) {
            fmt::print("    dependencies:\n");
            for (auto const& d : dependencies) {
                fmt::print("    - name: {}\n", d->getName());
                fmt::print("      path: {}\n", d->getPath());
            }
        }
        if (not project.getLegacyIncludePaths().empty()) {
            fmt::print("    includePath:\n");
            for (auto const& p : project.getLegacyIncludePaths()) {
                fmt::print("    - {}\n", p);
            }
        }
    }
}
void printTranslationSetTree(std::map<TranslationSet const*, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>>> const& _projects) {
    auto parentCt = std::map<TranslationSet const*, int>{};
    for (auto const& [i_project, dep] : _projects) {
        auto const& dependencies = std::get<0>(dep);
        for (auto const& d : dependencies) {
            parentCt[d] += 1;
        }
    }

    auto print = std::function<void(TranslationSet const*, std::string)>{};
    print = [&](TranslationSet const* project, std::string indent) {
        fmt::print("{}{}\n", indent, project->getName());
        indent += "  ";
        auto const& dependencies = std::get<0>(_projects.at(project));
        for (auto const& d : dependencies) {
            print(d, indent);
        }
    };

    for (auto const& [i_project, dep] : _projects) {
        if (parentCt[i_project] == 0) {
            print(i_project, "");
        }
    }
}

auto selectRootTranslationSets(std::set<std::string> const& names, std::map<TranslationSet const*, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>>> const& _projects) -> std::set<TranslationSet const*> {
    auto ret = std::set<TranslationSet const*>{};

    auto knownNames = std::set<std::string>{};

    auto parentCt = std::map<TranslationSet const*, int>{};
    for (auto const& [i_project, dep] : _projects) {
        auto const& dependencies = std::get<0>(dep);
        if (names.count(i_project->getName()) > 0) {
            knownNames.insert(i_project->getName());
        }
        for (auto const& d : dependencies) {
            parentCt[d] += 1;
        }
    }

    if (knownNames.size() != names.size()) {
        auto str = fmt::format("unknown target(s)");
        for (auto n : names) {
            if (knownNames.count(n) == 0) {
                str = fmt::format("{} {}", str, n);
            }
        }

        throw std::runtime_error(str);
    }

    auto queue = std::function<void(TranslationSet const*)>{};
    queue = [&](TranslationSet const* project) {
        ret.insert(project);
        auto const& dependencies = std::get<0>(_projects.at(project));
        for (auto const& d : dependencies) {
            queue(d);
        }
    };

    for (auto const& [i_project, dep] : _projects) {
        if (names.count(i_project->getName()) > 0) {
            queue(i_project);
        }
    }
    return ret;
}



auto loadConfig(std::filesystem::path const& workPath, std::filesystem::path const& buildPath, std::tuple<bool, std::filesystem::path> const& busyPath) -> Config {

    if (relative(buildPath) != ".") {
        if (not exists(buildPath)) {
            std::filesystem::create_directories(buildPath);
        }
        std::filesystem::current_path(buildPath);
        fmt::print("changing working directory to {}\n", buildPath);
    }

    auto config = [&]() {
        if (exists(global_busyConfigFile)) {
            return fon::yaml::deserialize<Config>(YAML::LoadFile(global_busyConfigFile));
        }
        return Config{};
    }();
    if (std::get<0>(busyPath) or config.rootDir.empty()) {
        auto rootDir = std::get<1>(busyPath);
        rootDir.remove_filename();
        config.rootDir  = relative(workPath / rootDir);
        config.busyFile = relative(workPath / std::get<1>(busyPath), config.rootDir);
    }

    if (config.rootDir.lexically_normal() == ".") {
        throw std::runtime_error("can't build in source, please create a `build` directory");
    }

    return config;
}

auto updateToolchainOptions(Config& config, bool reset, std::vector<std::string> const& _options) -> std::map<std::string, std::vector<std::string>> {
    auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);

    // initialize queue
    auto queue = std::queue<std::string>{};
    auto processed = std::set<std::string>{};
    if (reset) {
        queue.push("default");
        config.toolchain.options.clear();
    }
    for (auto const& o : _options) {
        queue.push(o);
    }
    while(not queue.empty()) {
        auto o = queue.front();
        queue.pop();

        auto [opt, act] = [&]() -> std::tuple<std::string, bool> {
            if (o.length() > 3 and o.substr(0, 3) == "no-") {
                return {o.substr(3), false};
            }
            return {o, true};
        }();

        if (processed.count(o) > 0) continue;
        processed.insert(o);

        auto iter = toolchainOptions.find(opt);
        if (iter == toolchainOptions.end()) {
            fmt::print("unknown toolchain option {} (removed)\n", o);
        } else {
            if (act) {
                config.toolchain.options.insert(opt);
                for (auto const& o2 : iter->second) {
                    queue.push(o2);
                }
            } else {
                config.toolchain.options.erase(opt);
            }
        }
    }
    return toolchainOptions;
}

auto computeEstimationTimes(TranslationSetMap const& projects_with_deps, bool clean, std::string const& _compilerHash, std::size_t jobs) -> std::tuple<ConsolePrinter::EstimatedTimes, std::chrono::milliseconds> {
    auto estimatedTimes = ConsolePrinter::EstimatedTimes{};


    using G  = Graph<TranslationSet const, File const>;
    using Q  = Queue<G>;
    auto graph = [&]() {
        auto nodes = G::Nodes{};
        auto edges = G::Edges{};
        for (auto& [project, dep] : projects_with_deps) {
            nodes.push_back(project);
            for (auto& file : project->getFiles()) {
                nodes.emplace_back(&file);
                edges.emplace_back(G::Edge{&file, project});
            }
            for (auto& d : std::get<0>(dep)) {
                edges.emplace_back(G::Edge{d, project});
            }
        }
        return G{std::move(nodes), std::move(edges)};
    }();
    auto queue = Q{graph};


    struct Thread {
        decltype(queue.pop()) work{nullptr};
        std::chrono::milliseconds finishesAt{};
    };
    auto threads = std::vector<Thread>(jobs);
    auto sortThreads = [&]() {
        std::sort(begin(threads), end(threads), [](auto const& lhs, auto const& rhs) {
            return lhs.finishesAt < rhs.finishesAt;
        });
    };

    auto queueAWorkLoad = [&](auto work) {
        auto& t = threads.front();
        if (t.work != nullptr) {
            queue.dispatch(t.work, [](auto) {});
        }
        auto duration = std::chrono::milliseconds{};

        queue.visit(work, overloaded {
            [&](busy::File const& file) {
                auto& fileInfo = getFileInfos().get(file.getPath());
                if (clean
                    or (fileInfo.hasChanged() and fileInfo.compilable)
                    or fileInfo.compilerHash != _compilerHash) {
                    estimatedTimes.try_emplace(&file, fileInfo.compileTime);
                    duration = fileInfo.compileTime;
                }
                return CompilePipe::Color::Compilable;
            }, [&](busy::TranslationSet const& project) {

                auto deps = std::unordered_set<TranslationSet const*>{};
                graph.visit_incoming<TranslationSet const>(&project, [&](TranslationSet const& project) {
                    deps.insert(&project);
                });

                auto& fileInfo = getFileInfos().get(project.getPath());
                auto anyChanges = [&]() {
                    for (auto const& d : deps) {
                        if (estimatedTimes.count(d) > 0) {
                            return true;
                        }
                    }
                    for (auto const& file : project.getFiles()) {
                        if (estimatedTimes.count(&file) > 0) {
                            return true;
                        }
                    }
                    return false;
                };
                auto existsAllOutputFiles = [&]() {
                    for (auto const& outputFile : fileInfo.outputFiles) {
                        if (not exists(outputFile)) {
                            return false;
                        }
                    }
                    return true;
                }();
                if (clean
                    or (not existsAllOutputFiles and fileInfo.compilable)
                    or anyChanges()
                    or fileInfo.compilerHash != _compilerHash) {
                    estimatedTimes.try_emplace(&project, fileInfo.compileTime);
                    duration = fileInfo.compileTime;
                }
                return CompilePipe::Color::Compilable;
            }
        });


        t.work = work;
        t.finishesAt += duration;
        sortThreads();
    };

    while (!queue.empty()) {
        auto next = queue.pop();
        queueAWorkLoad(next);
        auto activeThreads = std::accumulate(begin(threads), end(threads), 0, [](auto a, auto const& t) { return a + t.work?1:0; });
        while (queue.empty() and activeThreads > 0) {
            for (auto& t : threads) {
                if (t.work) {
                    queue.dispatch(t.work, [](auto) {});
                    for (auto& t2 : threads) {
                        t2.finishesAt = std::max(t2.finishesAt, t.finishesAt);
                    }
                    t.work = nullptr;
                    activeThreads = std::accumulate(begin(threads), end(threads), 0, [](auto a, auto const& t) { return a + t.work?1:0; });
                    sortThreads();
                    break;
                }
            }
        }
    }

    return {estimatedTimes, threads.back().finishesAt};
}


auto execute(std::vector<std::string> params, bool verbose) -> std::string {
    if (verbose) {
        params.emplace_back("--verbose");
    }
    auto call = std::stringstream{};
    for (auto const& p : params) {
        call << p << " ";
    }
    if (verbose) {
        fmt::print("call: {}\n", call.str());
    }

    auto p = process::Process{params};

    if (not verbose and p.getStatus() != 0) {
        fmt::print("call: {}\n", call.str());
    }
    if (verbose or p.getStatus() != 0) {
        if (not p.cout().empty()) fmt::print(std::cout, "{}\n", p.cout());
        if (not p.cerr().empty()) fmt::print(std::cerr, "{}\n", p.cerr());
    }
    if (p.getStatus() != 0) {
        throw CompileError{};
    }
    return p.cout();
}

void visitFilesWithWarnings(Config const& config, TranslationSetMap const& projects_with_deps, std::function<void(File const&, FileInfo const&)> fileF, std::function<void(TranslationSet const&, FileInfo const&)> projectF) {
    auto pipe = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options, config.sharedLibraries};
    while (not pipe.empty()) {
        auto work = pipe.pop();
        pipe.dispatch(work, overloaded {
            [&](busy::File const& file, auto const& /*params*/, auto const& /*deps*/) {
                auto& fileInfo = getFileInfos().get(file.getPath());
                if (fileInfo.hasWarnings and fileF) {
                    fileF(file, fileInfo);
                }
                return CompilePipe::Color::Compilable;
            }, [&](busy::TranslationSet const& project, auto const& /*params*/, auto const& /*deps*/) {
                auto& fileInfo = getFileInfos().get(project.getPath());
                if (fileInfo.hasWarnings and projectF) {
                    projectF(project, fileInfo);
                }
                return CompilePipe::Color::Compilable;
            }
        });
    }
}

auto isInteractive() -> bool {
    static bool interactive = isatty(fileno(stdout));
    return interactive;

}

auto getTargetType(TranslationSet const& project, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>> const& deps, std::set<std::string> const& sharedLibraries) -> TargetType {
    bool isExecutable = [&]() {
        if (project.getType() == "library") {
            return false;
        }
        return std::get<1>(deps).empty();
    }();
    if (isExecutable) {
        return TargetType::Executable;
    }
    if (sharedLibraries.count(project.getName()) > 0) {
        return TargetType::SharedLibrary;
    }
    return TargetType::StaticLibrary;
}

auto explode(std::string const& _str, std::vector<char> const& _dels) -> std::vector<std::string> {
    auto retList = std::vector<std::string>{};
    auto buffer = std::string{};
    for (auto c : _str) {
        auto has_c = any_of(begin(_dels), end(_dels), [=](char _c) { return _c == c; });
        if (not has_c) {
            buffer += c;
        } else if (has_c and not buffer.empty()) {
            retList.emplace_back(std::move(buffer));
            buffer = "";
        }
    }
    if (not buffer.empty()) {
        retList.emplace_back(std::move(buffer));
    }
    return retList;
}

void safeFileWrite(std::filesystem::path const& _dest, std::filesystem::path const& _src) {
    int fd = ::open(_src.string().c_str(), O_APPEND);
    ::fsync(fd);
    ::close(fd);
    ::rename(_src.string().c_str(), _dest.string().c_str());
}

auto exceptionToString(std::exception const& e, int level) -> std::string {
    std::string ret = std::string(level, ' ') + e.what();
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& nested) {
        ret += "\n" + exceptionToString(nested, level+1);
    } catch(...) {
        ret += "\nprintable exception";
    }
    return ret;
}

auto readFullFile(std::filesystem::path const& file) -> std::vector<std::byte> {
    auto ifs = std::ifstream{file, std::ios::binary};
    ifs.seekg(0, std::ios::end);
    std::size_t size = ifs.tellg();
    auto buffer = std::vector<std::byte>(size);
    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    return buffer;
}



}
