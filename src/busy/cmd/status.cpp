#include "../FileCache.h"
#include "../MultiCompilePipe.h"
#include "../cache.h"
#include "../config.h"
#include "../overloaded.h"
#include "../toolchains.h"
#include "../utils.h"

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <sargparse/File.h>
#include <sargparse/Parameter.h>
#include <sargparse/ArgumentParsing.h>
#include <unistd.h>

namespace busy::cmd {
namespace {

void status() {
	bool isInteractive = isatty(fileno(stdout));

	auto workPath   = std::filesystem::current_path();
	auto config     = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});
	auto cacheGuard = loadFileCache(*cfgYamlCache);

	auto [projects, packages] = busy::readPackage(config.rootDir, ".");

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	auto fg_green  = isInteractive? fg(fmt::terminal_color::green): fmt::text_style{};
	auto fg_yellow = isInteractive? fg(fmt::terminal_color::yellow): fmt::text_style{};
	auto fg_red    = isInteractive? fg(fmt::terminal_color::red): fmt::text_style{};

	fmt::print("using toolchain {} ({})\n",
		fmt::format(fg_green, "{}", config.toolchain.name),
		config.toolchain.call);
	fmt::print("  with options: {}\n",
		fmt::format(fg_yellow, "{}", fmt::join(config.toolchain.options, " ")));

	// check consistency of packages
	auto consistency = listConsistencyIssues(projects);
	if (consistency.empty()) {
		fmt::print("all projects are {}\n",
			fmt::format(fg_green, "{}", "consistent"));
	} else {
		fmt::print("{}\n",
			fmt::format(fg_red, "{} consistency issues", consistency.size()));
		for (auto [p1, p2] : consistency) {
			fmt::format("  - {}: {} and {} are not the same\n", p1->getName(), p1->getPath(), p2->getPath());
		}
	}
	auto projects_with_deps = createProjects(projects);

	auto jobs = [&]() -> int {
		if (*cfgJobs == 0) {
			return std::thread::hardware_concurrency();
		}
		return *cfgJobs;
	}();

	bool rebuild = cfgRebuild;
	{
		auto cout = execute({config.toolchain.call, "begin", config.rootDir}, false);
		auto node = YAML::Load(cout);
		rebuild = YAML::Node{node["rebuild"]}.as<bool>(rebuild);
		jobs = std::min(jobs, YAML::Node{node["max_jobs"]}.as<int>(jobs));
	}

	auto [_estimatedTimes, _estimatedTotalTime] = computeEstimationTimes(config, projects_with_deps, rebuild, jobs);
	auto estimatedTimes     = _estimatedTimes;
	auto estimatedTotalTime = _estimatedTotalTime;
	if (estimatedTimes.empty()) {
		fmt::print("{} files need processing\n", fmt::format(fg_green, "{}", 0));
	} else {
		fmt::print("{} files need processing\n", fmt::format(fg_red, "{}", estimatedTimes.size()));
		fmt::print("this will take {} using {} threads\n",
			fmt::format(fg_yellow, "{:0.1f}s", (estimatedTotalTime.count())/1000.f),
			fmt::format(fg_yellow, "{}", jobs));

	}
}

auto cmd = sargp::Command("status", "show status", status);

}
}
