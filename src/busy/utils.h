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
struct FileLock {
	int fd{-1};
	std::filesystem::path fullPath;
	FileLock(std::filesystem::path const& buildPath);
	~FileLock();
};


void printProjects(std::map<Project const*, std::tuple<std::set<Project const*>, std::set<Project const*>>> const& _projects);
auto loadConfig(std::filesystem::path const& workPath, std::filesystem::path const& buildPath, std::tuple<bool, std::filesystem::path> const& rootPath) -> Config;

auto updateToolchainOptions(Config& config, bool reset, std::vector<std::string> const& _options) -> std::map<std::string, std::vector<std::string>>;

auto computeEstimationTimes(Config const& config, ProjectMap const& projects_with_deps, bool clean, std::size_t jobs) -> std::tuple<ConsolePrinter::EstimatedTimes, std::chrono::milliseconds> ;

auto execute(std::vector<std::string> params, bool verbose) -> std::string;

void visitFilesWithWarnings(Config const& config, ProjectMap const& projects_with_deps, std::function<void(File const&, FileInfo const&)> fileF, std::function<void(Project const&, FileInfo const&)> projectF);
auto isInteractive() -> bool;

}
