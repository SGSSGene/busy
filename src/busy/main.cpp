
#include "commands/commands.h"
#include "Workspace.h"
#include "version.h"

#include <busyUtils/busyUtils.h>
#include <commonOptions/commonOptions.h>
#include <cstdio>
#include <sys/sysinfo.h>

#define TERM_RESET                      "\033[0m"

namespace {
	auto cmdBuild      = commonOptions::make_command("build",     "", "builds a specific project");
	auto cmdBuildMode  = commonOptions::make_command("buildMode", "", "Changes current buildmode to release or debug");
	auto cmdClang      = commonOptions::make_command("clang",         "Generates a .clang_complete file");
	auto cmdClean      = commonOptions::make_command("clean",         "Cleans current build and .busy directory");
	auto cmdCleanAll   = commonOptions::make_command("cleanall",      "deletes all build files");
	auto cmdDocu       = commonOptions::make_command("docu",          "Generates html docu");
	auto cmdEclipse    = commonOptions::make_command("eclipse",       "Generate Eclipse .project and .cproject file");
	auto cmdFlavors    = commonOptions::make_command("flavors",       "list all available flavors");
	auto cmdGit        = commonOptions::make_command("git", std::vector<std::string>{},  "executes given options on every repository (including root package)");
	auto cmdInfo       = commonOptions::make_command("info", std::vector<std::string>{}, "Infos about the current package and its dependencies");
	auto cmdInstall    = commonOptions::make_command("install",       "Installs the script to the current target");
	auto cmdLsFiles    = commonOptions::make_command("ls-files",      "Print all files of these repositories");
	auto cmdLsProjects = commonOptions::make_command("ls-projects",   "Print all projects in all repositories");
	auto cmdPull       = commonOptions::make_command("pull",          "Execute pull on all git repositories");
	auto cmdPush       = commonOptions::make_command("push",          "Execute push on all git repositories");
	auto cmdQF         = commonOptions::make_command("qf",            "Quickfixes busy.yaml");
	auto cmdQuickFix   = commonOptions::make_command("quickfix",      "Quickfixes busy.yaml");
	auto cmdRelPath    = commonOptions::make_command("showRelPath",   "Show the relative path to the root busy.yaml file");
	auto cmdShowDep    = commonOptions::make_command("showDep","",    "Show projects that depends directory on the given project");
	auto cmdStatus     = commonOptions::make_command("status",        "Shows current status of git repositories");
	auto cmdTest       = commonOptions::make_command("test",          "Run all unittests");
	auto cmdToolchain  = commonOptions::make_command("toolchain", "", "Changes current toolchain");
	auto cmdToolchains = commonOptions::make_command("toolchains",    "Shows available toolchain");


	auto swtHelp       = commonOptions::make_switch("help",        "Shows some inforamation about this program");
	auto swtNoConsole  = commonOptions::make_switch("noterminal",  "Doesn't use pretty output to display current progress");
	auto swtVerbose    = commonOptions::make_switch("verbose",     "Shows more information while running");
	auto swtVersion    = commonOptions::make_switch("version",     "Shows version");

	auto optFlavor     = commonOptions::make_option("flavor", "",   "builds and sets given flavor for future builds (toolchain + buildMode)");
	auto optJobCt      = commonOptions::make_option("j",        0,  "change the amount of jobs, 0 will autodetect good size");
}
using Action = std::function<void()>;

/**
 * check current working dir, and changes, if this is very likely to be
 * the wrong one. idea is to always endup in the cwd where busy.yaml is
 * located
 */
auto checkCwd() -> std::string {
	std::string relPath = ".";
	auto cwd = utils::cwd();
	while (cwd != "/" and not utils::fileExists("busy.yaml")) {
		utils::cwd("..");
		relPath = relPath + "/..";
		cwd = utils::cwd();
		if (cwd == "/") {
			throw std::runtime_error("this is not a busy repository");
		}
	}
	auto dirs = utils::explode(cwd, "/");
	if (dirs.size() > 1) {
		if (dirs[dirs.size()-2] == "extRepositories"
		    and utils::fileExists("../../busy.yaml")) {
			utils::cwd("../..");
			relPath = relPath + "/../..";
		}
	}
	return relPath;
}

int main(int argc, char** argv) {
	try {
		if (not commonOptions::parse(argc, argv)) {
			commonOptions::print();
			return EXIT_SUCCESS;
		} else if (*swtHelp) {
			commonOptions::print();
			return EXIT_SUCCESS;
		}
		if (*swtVersion) {
			std::cout << busy::version() << std::endl;
			return EXIT_SUCCESS;
		}

		if (not *swtNoConsole) {
			std::cout << TERM_RESET;
		}

		std::string relPath = ".";
		if (not *cmdLsFiles) {
			relPath = checkCwd();
		}

		if (*optFlavor != "") {
			busy::Workspace ws;
			ws.setFlavor(*optFlavor);
		}

		// setting good thread amount
		if (*optJobCt == 0) {
			struct sysinfo memInfo;
			sysinfo (&memInfo);
			int nprocs      = std::thread::hardware_concurrency();
			int memoryInGig = memInfo.totalram / 1000 / 1000 / 1000;

			int count = std::min(nprocs, memoryInGig);
			if (*swtVerbose) {
				std::cout << "setting jobs to " << count << std::endl;
			}
			optJobCt.setValue(count);
		}

		if (*cmdDocu) {
			commands::docu();
		} else if (*cmdTest) {
			commands::test();
		} else if (*cmdClean) {
			commands::clean();
		} else if (*cmdCleanAll) {
			commands::cleanAll();
		} else if (cmdGit->size() > 0) {
			commands::git(*cmdGit);
		} else if (*cmdEclipse) {
			commands::eclipse();
		} else if (*cmdFlavors) {
			commands::flavors(not *swtNoConsole);
		} else if (*cmdPull) {
			commands::pull();
		} else if (*cmdPush) {
			commands::push();
		} else if (*cmdRelPath) {
			std::cout<<relPath<<std::endl;
		} else if (cmdInfo->size() > 0) {
			commands::info(*cmdInfo);
		} else if (*cmdInstall) {
			commands::install();
		} else if (*cmdQuickFix or *cmdQF) {
			commands::quickFix();
		} else if (*cmdStatus) {
			commands::status();
		} else if (*cmdBuildMode != "") {
			commands::buildMode(*cmdBuildMode);
		} else if (*cmdLsFiles) {
			int exitCode = commands::listFiles(relPath + "/");
			exit(exitCode);
			return exitCode;
		} else if (*cmdLsProjects) {
			commands::listProjects();
		} else if (*cmdToolchains) {
			commands::toolchains(not *swtNoConsole);
		} else if (*cmdToolchain != "") {
			commands::toolchain(*cmdToolchain);
		} else if (*cmdClang) {
			commands::clang();
		} else if (*cmdShowDep != "") {
			commands::showDep(*cmdShowDep);
		} else {
			auto success = commands::build(*cmdBuild, *swtVerbose, *swtNoConsole, *optJobCt);
			if (not success) return EXIT_FAILURE;
		}
	} catch(std::runtime_error const& e) {
		std::cerr<<"exception(runtime_error): " << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch(std::exception const& e) {
		std::cerr << "exception(exception): " << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch(std::string const& s) {
		std::cerr << "exception(string): " << s << std::endl;
		return EXIT_FAILURE;
	} catch(char const* s) {
		std::cerr << "exception(char): " << s << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
