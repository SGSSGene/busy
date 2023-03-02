#include "Arguments.h"
#include "Desc.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"
#include "utils.h"

#include <cctype>

namespace {
auto _ = cliModeStatus.run([]() {
    auto workspace = Workspace{*cliBuildPath};
    updateWorkspace(workspace);

    // load busy files
    auto toolchains = loadAllBusyFiles(workspace, cliVerbose);

    updateWorkspaceToolchains(workspace, toolchains);

    auto allSets = std::vector<std::tuple<std::string, busy::desc::TranslationSet const*>>{};
    for (auto const& [key, ts] : workspace.allSets) {
        allSets.emplace_back(key, &ts);
    }

    std::ranges::sort(allSets, [](auto const& lhs, auto const& rhs) {
        auto const& lts = *std::get<1>(lhs);
        auto const& rts = *std::get<1>(rhs);

        auto lstr = std::string{std::get<0>(lhs)};
        auto rstr = std::string{std::get<0>(rhs)};
        std::ranges::transform(lstr, lstr.begin(), ::tolower);
        std::ranges::transform(rstr, rstr.begin(), ::tolower);

        return std::tie(lts.precompiled, lts.installed, lstr) <
               std::tie(rts.precompiled, rts.installed, rstr);
    });


    fmt::print("available ts:\n");
    for (auto type : {"executable", "library"}) {
        fmt::print("  {}:\n", type);
        for (auto const& [key, ts] : allSets) {
            if (ts->type != type) continue;
            fmt::print("    - {}{}{}\n", ts->name, ts->precompiled?" (precompiled)":"", ts->installed?" (installed)":"");
        }
    }
    fmt::print("available toolchains:\n");
    for (auto [key, value] : toolchains) {
        fmt::print("    - {}: {}\n", key, value);
    }
    workspace.save();
    exit(0);
});

}
