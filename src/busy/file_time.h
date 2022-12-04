#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <sys/stat.h>


struct file_time_t {
    static auto now() -> std::chrono::system_clock::time_point {
        using namespace std::chrono;
        auto t = std::chrono::file_clock::now();
        return file_clock::to_sys(t);
    }

    auto operator()(std::filesystem::path const& p) -> std::chrono::system_clock::time_point{
        using namespace std::chrono;
        auto t = last_write_time(p);
        return file_clock::to_sys(t);
    }
};
inline file_time_t file_time{};
