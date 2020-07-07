#pragma once

#include "completion.h"

#include <sargparse/ArgumentParsing.h>
#include <sargparse/File.h>
#include <sargparse/Parameter.h>

namespace busy {

inline auto cfgVerbose   = sargp::Flag{"verbose", "verbose output"};
inline auto cfgRootPath  = sargp::Parameter<sargp::Directory>{"..", "root",  "path to directory containing busy.yaml"};
inline auto cfgBuildPath = sargp::Parameter<sargp::Directory>{".",  "build", "path to build directory"};
inline auto cfgJobs      = sargp::Parameter<int>{0, "jobs", "thread count"};
inline auto cfgClean     = sargp::Flag{"clean", "clean build, using no cache"};
inline auto cfgYamlCache = sargp::Flag{"yaml-cache", "save cache in yaml format"};
inline auto cfgToolchain = sargp::Parameter<std::string>{"", "toolchain", "set toolchain", []{}, &comp::toolchain};
inline auto cfgOptions   = sargp::Parameter<std::vector<std::string>>{{}, "option", "options for toolchains", []{}, &comp::options};

}
