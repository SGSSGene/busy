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

void printProjects(std::map<analyse::Project const*, std::tuple<std::set<analyse::Project const*>, std::set<analyse::Project const*>>> const& _projects);
auto loadConfig(std::filesystem::path workPath, std::optional<std::filesystem::path> rootPath) -> Config;
auto updateToolchainOptions(Config& config, bool reset, std::optional<std::vector<std::string>> _options) -> std::map<std::string, std::vector<std::string>>;

auto computeEstimationTimes(Config const& config, analyse::ProjectMap const& projects_with_deps, bool clean) -> ConsolePrinter::EstimatedTimes;

auto execute(std::vector<std::string> const& params, bool verbose) -> std::string;

}
