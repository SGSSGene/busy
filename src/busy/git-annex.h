#pragma once

#include <busyUtils/busyUtils.h>

namespace git {
namespace annex {

auto sync(std::string const& _cwd) -> std::string;
auto sync_content(std::string const& _cwd) -> std::string;

}
}

