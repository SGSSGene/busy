#include <sargparse/ArgumentParsing.h>
#include <sargparse/File.h>
#include <sargparse/Parameter.h>

#include <fmt/format.h>

namespace busy::cmd {
namespace {

auto version() {
	fmt::print("busy 2.0.0-git-alpha\n");
	fmt::print("Copyright (C) 2020 Simon Gene Gottlieb\n");
}

auto cmd = sargp::Flag{"version", "show version", &version};

}
}
