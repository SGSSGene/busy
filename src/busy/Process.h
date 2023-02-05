#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <vector>

namespace process {

class Process final {
private:
    static constexpr int READ_END{0};
    static constexpr int WRITE_END{1};

    std::array<int, 2> stdoutpipe;
    std::array<int, 2> stderrpipe;
    int status;

    std::vector<char> stdcout;
    std::vector<char> stdcerr;
public:
    Process(std::span<std::string> prog, std::filesystem::path const& _cwd = std::filesystem::current_path()) {
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

    [[nodiscard]] auto cout() const { return std::string_view{stdcout.begin(), stdcout.end()}; }
    [[nodiscard]] auto cerr() const { return std::string_view{stdcerr.begin(), stdcerr.end()}; }
    [[nodiscard]] auto getStatus() const -> int { return status; }
private:
    void childProcess(std::span<std::string> _prog) {
        auto envPath = std::string{getenv("PATH")};
        auto execStr = [&]() -> std::string {
            for (auto s : std::views::split(envPath, ":")) {
                auto _s = std::filesystem::path{s.begin(), s.end()};
                if (std::filesystem::exists(_s / _prog[0])) {
                    return _s / _prog[0];
                }
            }
            return _prog[0];
        }();

        auto argv = std::vector<char*>{};
        for (auto const& a : _prog) {
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
            while (true) {
                auto origSize = out.size();
                if (out.capacity() - out.size() > 4096) {
                    out.resize(out.capacity() * 2);
                } else {
                    out.resize(out.capacity());
                }
                auto size = read(file, out.data() + origSize, out.size() - origSize);
                if (size <= 0) {
                    out.resize(origSize);
                    return;
                }
                out.resize(origSize + size);
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
