#include "../FileCache.h"
#include "../MultiCompilePipe.h"
#include "../cache.h"
#include "../config.h"
#include "../overloaded.h"
#include "../toolchains.h"
#include "../utils.h"
#include "../utils/utils.h"

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <sargparse/sargparse.h>
#include <unistd.h>

namespace busy::cmd {
namespace {

void status();
auto cmd         = sargp::Command("status", "show status", status);
auto cfgFiles    = cmd.Parameter<std::vector<std::filesystem::path>>({}, "", "show warnings of given files", []{}, sargp::completeFile("", sargp::File::Multi));
auto cfgAllFiles = cmd.Flag("all_files", "show warnings of all files with warnings");

void status() {
	auto workPath   = std::filesystem::current_path();
	auto config     = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});
	auto cacheGuard = loadFileCache(*cfgYamlCache);

	auto [projects, packages] = busy::readPackage(config.rootDir, ".");

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	auto fg_green  = isInteractive()? fg(fmt::terminal_color::green): fmt::text_style{};
	auto fg_yellow = isInteractive()? fg(fmt::terminal_color::yellow): fmt::text_style{};
	auto fg_red    = isInteractive()? fg(fmt::terminal_color::red): fmt::text_style{};

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

	auto jobs = [&]() -> std::size_t {
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
		jobs = std::min(jobs, YAML::Node{node["max_jobs"]}.as<std::size_t>(jobs));
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
	for (auto const& [target, time] : estimatedTimes) {
		std::visit(overloaded{
			[](File const* file) {
				fmt::print("  - {}\n", file->getPath());
			}, [](Project const* project) {
				fmt::print("  - {}\n", project->getName());
			}
		}, target);
	}

	auto echo = [](std::filesystem::path filePath, std::string const& ext) {
		filePath = "obj" / filePath;
		filePath.replace_extension(ext);
		auto data = utils::readFullFile(filePath);
		data.push_back(std::byte(0));
		fmt::print("{}\n", (char*)data.data());
	};


	int warnings = 0;
	visitFilesWithWarnings(config, projects_with_deps, [&](File const& file, FileInfo const&) {
		warnings += 1;
		if (cfgAllFiles) {
			auto filePath = file.getPath();
			fmt::print("{}:\n", filePath);
			echo(filePath, ".o.stdout");
			echo(filePath, ".o.stderr");
		}
	}, nullptr);

	for (auto const& filePath : *cfgFiles) {
		fmt::print("{}:\n", filePath);
		echo(filePath, ".o.stdout");
		echo(filePath, ".o.stderr");
	}


	if (warnings == 0) {
		fmt::print("all files {}\n", fmt::format(fg_green, "warning free"));
	} else {
		fmt::print("{} files with {}\n", warnings, fmt::format(fg_red, "warnings"));
		if (not cfgFiles) {
			visitFilesWithWarnings(config, projects_with_deps, [&](File const& file, FileInfo const&) {
				fmt::print("  - {}\n", file.getPath());
			}, nullptr);
		}
	}
}

}
}
