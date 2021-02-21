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
    struct Infos {
        template <typename Node2>
        static void convert(Node2& node, std::filesystem::path& path) {
            std::string str = path.string();
            node % str;
            path = str;
        }
    };
    convert(Node&, std::filesystem::path&) {}
};

}

