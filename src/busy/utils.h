#pragma once

#include "ConsolePrinter.h"
#include "Project.h"
#include "config.h"
#include "CompilePipe.h"


#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <tuple>

namespace busy {

struct CompileError {};

void printProjects(std::map<Project const*, std::tuple<std::set<Project const*>, std::set<Project const*>>> const& _projects);
auto loadConfig(std::filesystem::path workPath, std::filesystem::path buildPath, std::tuple<bool, std::filesystem::path> rootPath) -> Config;

auto updateToolchainOptions(Config& config, bool reset, std::optional<std::vector<std::string>> _options) -> std::map<std::string, std::vector<std::string>>;

auto computeEstimationTimes(Config const& config, ProjectMap const& projects_with_deps, bool clean, int jobs) -> std::tuple<ConsolePrinter::EstimatedTimes, std::chrono::milliseconds> ;

auto execute(std::vector<std::string> params, bool verbose) -> std::string;

}
