#include "compile.h"

#include "../overloaded.h"
#include "../utils.h"
#include "../FileCache.h"
#include "../cache.h"
#include "../toolchains.h"
#include "../MultiCompilePipe.h"

namespace busy::cmd {

void compile() {
	auto workPath = std::filesystem::current_path();
	auto config = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});
	auto cacheGuard = loadFileCache(*cfgYamlCache);

	auto [projects, packages] = busy::readPackage(config.rootDir, ".");

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);


	if (cfgToolchain) {
		auto toolchains = searchForToolchains(packages);
		auto iter = toolchains.find(*cfgToolchain);
		if (iter == toolchains.end()) {
			throw std::runtime_error("could not find toolchain \"" + config.toolchain.name + "\"");
		}

		config.toolchain.name = iter->first;
		config.toolchain.call = iter->second;
		updateToolchainOptions(config, true, cfgOptions);
		fmt::print("Setting toolchain to {} ({})\n", config.toolchain.name, config.toolchain.call);
	}
	auto toolchainOptions = updateToolchainOptions(config, false, cfgOptions);

	fmt::print("using toolchain {} ({})\n", config.toolchain.name, config.toolchain.call);
	fmt::print("  with options: {}\n", fmt::join(config.toolchain.options, " "));


	// check consistency of packages
	fmt::print("checking consistency...");
	checkConsistency(projects);
	fmt::print("done\n");

	auto projects_with_deps = createProjects(projects);

	// Save config
	{
		YAML::Emitter out;
		out << fon::yaml::serialize(config);
		std::ofstream(global_busyConfigFile) << out.c_str();
	}


	fmt::print("checking files...");
	auto jobs = [&]() -> int {
		if (*cfgJobs == 0) {
			return std::thread::hardware_concurrency();
		}
		return *cfgJobs;
	}();

	bool clean = cfgClean;
	{
		auto cout = execute({config.toolchain.call, "begin", config.rootDir}, false);
		auto node = YAML::Load(cout);
		clean = YAML::Node{node["clean"]}.as<bool>(clean);
		jobs = std::min(jobs, YAML::Node{node["max_jobs"]}.as<int>(jobs));
	}

	auto [_estimatedTimes, _estimatedTotalTime] = computeEstimationTimes(config, projects_with_deps, clean, jobs);
	auto estimatedTimes     = _estimatedTimes;
	auto estimatedTotalTime = _estimatedTotalTime;
	fmt::print("done\n");
	fmt::print("{} files need processing\n", estimatedTimes.size());
	if (not estimatedTimes.empty()) {

		fmt::print("start compiling...\n");
		auto consolePrinter = ConsolePrinter{estimatedTimes, estimatedTotalTime};
		auto pipe           = CompilePipe{config.toolchain.call, projects_with_deps, config.toolchain.options};

		auto multiPipe = MultiCompilePipe{pipe, jobs};

		std::mutex mutex;
		multiPipe.work(overloaded {
			[&](busy::File const& file, auto const& params, auto const& deps) {
				auto startTime  = std::chrono::file_clock::now();
				auto path       = file.getPath();

				auto g = std::unique_lock{mutex};
				auto& fileInfo = getFileInfos().get(path);
				g.unlock();
				if (estimatedTimes.count(&file) == 0) {
					return fileInfo.compilable?CompilePipe::Color::Compilable:CompilePipe::Color::Ignored;
				}

				consolePrinter.startJob(&file, "compiling " + path.string());

				fileInfo.needRecompiling = true;
				// store file date before compiling
				g.lock();
				auto hash    = getFileCache().getHash(path);
				g.unlock();

				// compiling
				auto cout = execute(params, cfgVerbose);
				auto node = YAML::Load(cout);

				bool cached         = YAML::Node{node["cached"]}.as<bool>(false);
				fileInfo.compilable = YAML::Node{node["compilable"]}.as<bool>(false);
				fileInfo.outputFile = YAML::Node{node["output_file"]}.as<std::string>("");

				auto dependencies = FileInfo::Dependencies{};
				//!TODO should work with `auto`, but doesn't for some reason
				for (YAML::Node n : node["dependencies"]) {
					auto path = FileInfo::Path{n.as<std::string>()};
					auto hash    = computeHash(path);
					auto modTime = getFileModificationTime(path);

					//!TODO handle when files changes inbetween
					if (modTime > startTime) {
						auto f = [](auto t) {
							return std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
						};
							fmt::print("file changed??? {} {} {} {} {}\n", f(modTime), f(startTime), path, std::filesystem::current_path(), hash);
					//		throw std::runtime_error("file changed");
						//status = 2; // file changed in between
					}
					dependencies.emplace_back(path, hash);
				}
				fileInfo.dependencies    = dependencies;
				fileInfo.hash            = hash;
				fileInfo.needRecompiling = false;

				auto compileTime = consolePrinter.finishedJob(&file);
				if (not cached or fileInfo.compileTime < compileTime) {
					fileInfo.compileTime = compileTime;
				}
				fileInfo.modTime = startTime;
				return fileInfo.compilable?CompilePipe::Color::Compilable:CompilePipe::Color::Ignored;
			}, [&](busy::Project const& project, auto const& params, auto const& deps) {
				auto& fileInfo = getFileInfos().get(project.getPath());

				if (estimatedTimes.count(&project) == 0) {
					return fileInfo.compilable?CompilePipe::Color::Compilable:CompilePipe::Color::Ignored;
				}

				consolePrinter.startJob(&project, "linking " + project.getName());
				auto cout = execute(params, cfgVerbose);
				auto node = YAML::Load(cout);
				bool cached         = YAML::Node{node["cached"]}.as<bool>(false);
				fileInfo.compilable = YAML::Node{node["compilable"]}.as<bool>(false);
				fileInfo.outputFile = YAML::Node{node["output_file"]}.as<std::string>("");

				auto compileTime = consolePrinter.finishedJob(&project);
				if (not cached or fileInfo.compileTime < compileTime) {
					fileInfo.compileTime = compileTime;
				}

				return fileInfo.compilable?CompilePipe::Color::Compilable:CompilePipe::Color::Ignored;
			}
		});
		multiPipe.join();
		if (multiPipe.compileError) {
			throw CompileError{};
		}
	}
	execute({config.toolchain.call, "end"}, false);
	fmt::print("done\n");
}
namespace {

auto _cmd        = sargp::Command{"compile", "compile everything (default)", &compile};
auto _cmdDefault = sargp::Task{[]{
	// only run if no children are active
	for (auto cmd : sargp::getDefaultCommand().getSubCommands()) {
		if (*cmd) return;
	}
	compile();
}};

}
}