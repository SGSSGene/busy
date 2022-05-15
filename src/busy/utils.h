#pragma once

#include "ConsolePrinter.h"
#include "TranslationSet.h"
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
    std::filesystem::path lockFile;
    FileLock();
    ~FileLock();
};


void printTranslationSets(std::map<TranslationSet const*, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>>> const& _projects);
void printTranslationSetTree(std::map<TranslationSet const*, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>>> const& _projects);
auto selectRootTranslationSets(std::set<std::string> const& names, std::map<TranslationSet const*, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>>> const& _projects) -> std::set<TranslationSet const*>;
auto loadConfig(std::filesystem::path const& workPath, std::filesystem::path const& buildPath, std::tuple<bool, std::filesystem::path> const& busyPath) -> Config;

auto updateToolchainOptions(Config& config, bool reset, std::vector<std::string> const& _options) -> std::map<std::string, std::vector<std::string>>;

auto computeEstimationTimes(TranslationSetMap const& projects_with_deps, bool clean, std::string const& _compilerHash, std::size_t jobs) -> std::tuple<ConsolePrinter::EstimatedTimes, std::chrono::milliseconds> ;

auto execute(std::vector<std::string> params, bool verbose) -> std::string;

void visitFilesWithWarnings(Config const& config, TranslationSetMap const& projects_with_deps, std::function<void(File const&, FileInfo const&)> fileF, std::function<void(TranslationSet const&, FileInfo const&)> projectF);
auto isInteractive() -> bool;

enum class TargetType { Executable, StaticLibrary, SharedLibrary };
auto getTargetType(TranslationSet const& project, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>> const& deps, std::set<std::string> const& sharedLibraries) -> TargetType;

auto explode(std::string const& _str, std::vector<char> const& _dels) -> std::vector<std::string>;

void safeFileWrite(std::filesystem::path const& _dest, std::filesystem::path const& _src);

auto exceptionToString(std::exception const& e, int level = 0) -> std::string;
auto readFullFile(std::filesystem::path const& file) -> std::vector<std::byte>;

auto getenv(std::string const& key) -> std::string;


}
