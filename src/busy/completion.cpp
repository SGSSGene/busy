#include "completion.h"

#include "utils.h"
#include "toolchains.h"

#include <filesystem>

namespace busy::comp {

auto toolchain(std::vector<std::string> const& /*str*/) -> std::pair<bool, std::set<std::string>> {
	auto ret = std::pair<bool, std::set<std::string>>{false, {}};
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});

	auto packages = std::vector<std::filesystem::path>{};
	if (!config.rootDir.empty() and config.rootDir != "." and std::filesystem::exists(config.rootDir)) {
		auto [pro, pack] = busy::readPackage(config.rootDir, ".");
		for (auto const& p : pack) {
			packages.emplace_back(p);
		}
	}

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);
	for (auto const&  [name, path] : searchForToolchains(packages)) {
		ret.second.insert(name);
	}
	return ret;
}

auto options(std::vector<std::string> const& /*str*/) -> std::pair<bool, std::set<std::string>> {
	auto ret = std::pair<bool, std::set<std::string>>{false, {}};
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});

	auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);
	for (auto const& opt : toolchainOptions) {
		if (config.toolchain.options.count(opt.first) == 0) {
			ret.second.insert(opt.first);
		} else {
			ret.second.insert("no-" + opt.first);
		}
	}
	return ret;
}


}
