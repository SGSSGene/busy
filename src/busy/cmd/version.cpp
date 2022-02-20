#include <fmt/format.h>
#include <sargparse/sargparse.h>

namespace busy::cmd {
namespace {

auto version() {
    fmt::print("busy 2.0.0-git-alpha\n");
    fmt::print("Copyright (C) 2020 Simon Gene Gottlieb\n");
}

auto cmd = sargp::Flag{"version", "show version", version};

}
}
