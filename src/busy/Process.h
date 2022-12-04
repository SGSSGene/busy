#pragma once
#include <array>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <sys/wait.h>

namespace process {

constexpr inline int READ_END = 0;
constexpr inline int WRITE_END = 1;

inline auto explode(std::string input, char c) -> std::vector<std::string> {
    auto retList = std::vector<std::string>{};
    std::istringstream str{input};
    for (std::string s; std::getline(str, s, c);) {
        retList.emplace_back(std::move(s));
    }
    return retList;
}

class Process final {
private:
    std::array<int, 2> stdoutpipe;
    std::array<int, 2> stderrpipe;
    int status;

    std::stringstream stdcout;
    std::stringstream stdcerr;
public:
    Process(std::vector<std::string> const& prog, std::filesystem::path const& _cwd = std::filesystem::current_path()) {
        int ret1 = pipe(stdoutpipe.data());
        int ret2 = pipe(stderrpipe.data());
        if (ret1 == -1 || ret2 == -1) {
            throw std::runtime_error("couldn't create pipes");
        }

        auto pid = fork();
        if (pid==0) {
            std::filesystem::current_path(_cwd);
            childProcess(prog);
        } else {
            parentProcess(pid);
        }
    }

    ~Process() {
        close(stdoutpipe[READ_END]);
        close(stderrpipe[READ_END]);
    }
    Process(Process const&) = delete;
    Process(Process&&) = delete;
    auto operator=(Process const&) -> Process& = delete;
    auto operator=(Process&&) -> Process& = delete;

    [[nodiscard]] auto cout() const { return stdcout.view(); }
    [[nodiscard]] auto cerr() const { return stdcerr.view(); }
    [[nodiscard]] auto getStatus() const -> int { return status; }
private:
    void childProcess(std::vector<std::string> const& _prog) {
        auto envPath = std::string{getenv("PATH")};
        auto execStr = [&]() -> std::string {
            for (std::filesystem::path _s : explode(envPath, ':')) {
                if (std::filesystem::exists(_s / _prog[0])) {
                    return _s / _prog[0];
                }
            }
            return _prog[0];
        }();

        std::vector<char*> argv;
        for (auto& a : _prog) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        dup2(stdoutpipe[WRITE_END], STDOUT_FILENO);
        close(stdoutpipe[READ_END]);

        dup2(stderrpipe[WRITE_END], STDERR_FILENO);
        close(stderrpipe[READ_END]);

        execv(execStr.c_str(), argv.data());
        exit(127); // this is only reached if execv fails
    }

    void parentProcess(pid_t pid) {
        auto readUntilEnd = [](auto& file, auto& out) {
            std::array<char, 4096> buffer;
            while (true) {
                if (auto size = read(file, buffer.data(), buffer.size()); size >= 0) {
                    out << std::string_view{buffer.data(), buffer.data()+size};
                    if (size == 0) return;
                } else return;
            }
        };
        auto t1 = std::jthread{[&] { readUntilEnd(stdoutpipe[READ_END], stdcout); }};
        auto t2 = std::jthread{[&] { readUntilEnd(stderrpipe[READ_END], stdcerr); }};

        waitpid(pid, &status, 0); /* wait for child to exit */
        status = status >> 8;
        close(stdoutpipe[WRITE_END]);
        close(stderrpipe[WRITE_END]);
    }
};

}
