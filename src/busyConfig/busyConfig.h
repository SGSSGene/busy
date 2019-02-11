#pragma once

#include "Package.h"

#include <filesystem>

namespace busyConfig {

auto readPackage(std::filesystem::path path) -> Package;

}
