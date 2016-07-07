
#include <commonOptions/commonOptions.h>
#include "commands/commands.h"

namespace {
	auto cmdBuild      = commonOptions::make_command("build",     "", "builds a specific project");
	auto cmdBuildMode  = commonOptions::make_command("buildMode", "", "Changes current buildmode to release or debug");
	auto cmdClang      = commonOptions::make_command("clang",         "Generates a .clang_complete file");
	auto cmdClean      = commonOptions::make_command("clean",         "Cleans current build and .aBuild directory");
	auto cmdCleanAll   = commonOptions::make_command("cleanall",      "deletes all build files");
	auto cmdDocu       = commonOptions::make_command("docu",          "Generates html docu");
	auto cmdEclipse    = commonOptions::make_command("eclipse",       "Generate Eclipse .project and .cproject file");
	auto cmdGit        = commonOptions::make_command("git", std::vector<std::string>{}, "executes given options on every repository (including root package)");
	auto cmdInstall    = commonOptions::make_command("install",       "Installs the script to the current target");
	auto cmdLsFiles    = commonOptions::make_command("ls-files",      "Print all files of these repositories");
	auto cmdPull       = commonOptions::make_command("pull",          "Execute pull on all git repositories");
	auto cmdPush       = commonOptions::make_command("push",          "Execute push on all git repositories");
	auto cmdQF         = commonOptions::make_command("qf",            "Quickfixes aBuild.yaml");
	auto cmdQuickFix   = commonOptions::make_command("quickfix",      "Quickfixes aBuild.yaml");
	auto cmdRelPath    = commonOptions::make_command("showRelPath",   "Show the relative path to the root aBuild.yaml file");
	auto cmdShowDep    = commonOptions::make_command("showDep","",    "Show projects that depends directory on the given project");
	auto cmdStatus     = commonOptions::make_command("status",        "Shows current status of git repositories");
	auto cmdTest       = commonOptions::make_command("test",          "Run all unittests");
	auto cmdToolchain  = commonOptions::make_command("toolchain", "", "Changes current toolchain");
	auto cmdToolchains = commonOptions::make_command("toolchains",    "Shows available toolchain");


	auto swtHelp       = commonOptions::make_switch("help",        "Shows some inforamation about this program");
	auto swtNoConsole  = commonOptions::make_switch("noterminal",  "Doesn't use pretty output to display current progress");
	auto swtVerbose    = commonOptions::make_switch("verbose",     "Shows more information while running");

	auto optFlavor     = commonOptions::make_option("flavor", "",   "builds and sets given flavor for future builds (toolchain + buildMode)");
	auto optJobCt      = commonOptions::make_option("j",       10, "changes the amount of jobs");

//	auto optClone      = commonOptions::make_multi_option("clone", {}, "clones given git repository");

}
using namespace aBuild;

using Action = std::function<void()>;

/**
 * check current working dir, and changes, if this is very likely to be
 * the wrong one. idea is to always endup in the cwd where aBuild.json is
 * located
 */
std::string checkCwd() {
	std::string relPath = ".";
	auto cwd = utils::cwd();
	while (cwd != "/" and not (utils::fileExists("aBuild.json")
	                           or utils::fileExists("aBuild.yaml"))) {
		utils::cwd("..");
		relPath = relPath + "/..";
		cwd = utils::cwd();
		if (cwd == "/") {
			throw std::runtime_error("this is not a aBuild repository/workspace");
		}
	}
	auto dirs = utils::explode(cwd, "/");
	if (dirs.size() > 1) {
		if (dirs[dirs.size()-2] == "extRepositories"
		    and (utils::fileExists("../../aBuild.json")
		         or utils::fileExists("../../aBuild.yaml"))) {
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
			return 0;
		} else if (*swtHelp) {
			commonOptions::print();
			return 0;
		}
		std::string relPath = ".";
		if (not *cmdLsFiles) {
			relPath = checkCwd();
		}

		// converting yaml files to json files
		if (utils::fileExists("aBuild.json")
			and not utils::fileExists("aBuild.yaml")) {
			Package package {PackageURL()};
			std::cout << "found aBuild.json converting to aBuild.yaml" << std::endl;
			serializer::json::read("aBuild.json", package);
			serializer::yaml::write("aBuild.yaml", package);
		} else if (utils::fileExists("aBuild.json")
				   and utils::fileExists("aBuild.yaml")) {
			std::cout << "found aBuild.json and aBuild.yaml, using aBuild.yaml." << std::endl;
		}
		if (*optFlavor != "") {
			Workspace ws(".");
			ws.accessConfigFile().setLastFlavor(*optFlavor);
			ws.save();
		}


		if (*cmdDocu) {
			commands::docu();
		} else if (*cmdTest) {
			commands::test();
		} else if (*cmdClean) {
			commands::clean();
		} else if (*cmdCleanAll) {
			commands::cleanAll();
/*		} else if (optClone->size() == 2) {
			commands::clone((*optClone)[0], (*optClone)[1] + "/");
		} else if (optClone->size() == 1) {
			std::string url  = (*optClone)[0];
			std::string path = (*optClone)[0];
			if (utils::isEndingWith(path, ".git")) {
				for (int i {0}; i<4; ++i) path.pop_back();
			}
			auto l = utils::explode(path, "/");
			path = l[l.size()-1];
			commands::clone(url, path + "/");*/
		} else if (cmdGit->size() > 0) {
			commands::git(*cmdGit);
		} else if (*cmdEclipse) {
			commands::eclipse();
		} else if (*cmdPull) {
			commands::pull();
		} else if (*cmdPush) {
			commands::push();
		} else if (*cmdRelPath) {
			std::cout<<relPath<<std::endl;
		} else if (*cmdInstall) {
			commands::install();
		} else if (*cmdQuickFix or *cmdQF) {
			commands::quickFix();
		} else if (*cmdStatus) {
			commands::status();
		} else if (*cmdBuildMode != "") {
			commands::status(*cmdBuildMode);
		} else if (*cmdLsFiles) {
			int exitCode = commands::listFiles(relPath + "/");
			exit(exitCode);
			return exitCode;
		} else if (*cmdToolchains) {
			commands::toolchains();
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
	} catch(std::runtime_error e) {
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
