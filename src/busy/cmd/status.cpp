#include <sargparse/ArgumentParsing.h>
#include <sargparse/File.h>
#include <sargparse/Parameter.h>

#include <fmt/format.h>

namespace busy::cmd {
namespace {

void status() {
	fmt::print("print status\n");
}

auto cmd = sargp::Command("status", "show status", status);

}
}
