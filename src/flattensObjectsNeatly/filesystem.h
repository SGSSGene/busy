#pragma once

#include "convert.h"

#include <filesystem>


// needs to include chrono for std::filesystem::file_time_type
#include "chrono.h"


namespace fon {

// convertible
template <typename Node>
struct convert<Node, std::filesystem::path> {
    static constexpr Type type = Type::Convertible;
    template <typename Node2>
    static void access(Node2& node, std::filesystem::path& path) {
        std::string str = path.string();
        node % str;
        path = str;
    }
    template <typename Node2>
    static auto access(Node2& node, std::filesystem::path const& path) {
        std::string str = path.string();
        return node % str;
    }
};

}

