#pragma once

#include "completion.h"

#include <filesystem>
#include <sargparse/sargparse.h>
#include <set>
#include <string>

namespace busy {

inline auto cfgVerbose    = sargp::Flag{"verbose", "verbose output"};
inline auto cfgBusyPath   = sargp::Parameter<std::filesystem::path>{"../busy.yaml", "",  "path to busy.yaml", []{}, sargp::completeFile(".yaml")};
inline auto cfgBuildPath  = sargp::Parameter<std::filesystem::path>{".",  "build", "path to build directory", []{}, sargp::completeDirectory()};
inline auto cfgJobs       = sargp::Parameter<int>{0, "jobs", "thread count"};
inline auto cfgRebuild    = sargp::Flag{"rebuild", "triggers all files to be rebuild"};
inline auto cfgYamlCache  = sargp::Flag{"yaml-cache", "save cache in yaml format"};
inline auto cfgToolchain  = sargp::Parameter<std::string>{"", "toolchain", "set toolchain", []{}, &comp::toolchain};
inline auto cfgOptions    = sargp::Parameter<std::vector<std::string>>{{}, "option", "options for toolchains", []{}, &comp::options};

//!TODO should follow XDG variables (see cppman) and may be be not hard coded?
inline static auto global_sharedPath = std::filesystem::path{"/usr/share/busy"};
inline static auto user_sharedPath   = []() {
	return std::filesystem::path{getenv("HOME")} / ".config/busy";
}();

inline static auto global_toolchainDir   = std::filesystem::path{"toolchains.d"};
inline static auto global_busyConfigFile = std::filesystem::path{".busy.yaml"};
inline static auto global_lockFile       = std::filesystem::path{".lock"};

struct Config {
	struct {
		std::string name {};
		std::string call {};
		std::set<std::string> options;
	} toolchain;

	std::string rootDir {};


	template <typename Node>
	void serialize(Node& node) {
		node["toolchain_name"]    % toolchain.name;
		node["toolchain_call"]    % toolchain.call;
		node["toolchain_options"] % toolchain.options;
		node["rootDir"]           % rootDir;
	}
};

}
