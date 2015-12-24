
#include <commonOptions/commonOptions.h>
#include "commands/commands.h"

namespace {
	auto swtDocu       = commonOptions::make_switch("docu",        "Generates html docu");
	auto swtClang      = commonOptions::make_switch("clang",       "Generates a .clang_complete file");
	auto swtClean      = commonOptions::make_switch("clean",       "Cleans current build and .aBuild directory");
	auto swtCleanAll   = commonOptions::make_switch("cleanall",    "deletes all build files");
	auto swtEclipse    = commonOptions::make_switch("eclipse",     "Generate Eclipse .project and .cproject file");
	auto swtHelp       = commonOptions::make_switch("help",        "Shows some inforamation about this program");
	auto swtInstall    = commonOptions::make_switch("install",     "Installs the script to the current target");
	auto swtLsFiles    = commonOptions::make_switch("ls-files",    "Print all files of these repositories");
	auto swtNoConsole  = commonOptions::make_switch("noterminal",  "Doesn't use pretty output to display current progress");
	auto swtPull       = commonOptions::make_switch("pull",        "Execute pull on all git repositories");
	auto swtPush       = commonOptions::make_switch("push",        "Execute push on all git repositories");
	auto swtQuickFix   = commonOptions::make_switch("quickfix",    "Quickfixes aBuild.yaml");
	auto swtQF         = commonOptions::make_switch("qf",          "Quickfixes aBuild.yaml");
	auto swtRelPath    = commonOptions::make_switch("showRelPath", "Show the relative path to the root aBuild.yaml file");
	auto swtStatus     = commonOptions::make_switch("status",      "Shows current status of git repositories");
	auto swtTest       = commonOptions::make_switch("test",        "Run all unittests");
	auto swtToolchains = commonOptions::make_switch("toolchains",  "Shows available toolchain");
	auto swtVerbose    = commonOptions::make_switch("verbose",     "Shows more information while running");

	auto optClone      = commonOptions::make_multi_option("clone", {}, "clones given git repository");
	auto optGit        = commonOptions::make_multi_option("git",   {}, "executes given options on every repository (including root package)");
	auto optBuild      = commonOptions::make_option("build", "", "builds a specific project");
	auto optFlavor     = commonOptions::make_option("flavor",    "", "Changes current flavor");
	auto optToolchain  = commonOptions::make_option("toolchain", "", "Changes current toolchain");

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
			throw std::runtime_error("this is not a aBuild packages/workspace");
		}
	}
	auto dirs = utils::explode(cwd, "/");
	if (dirs.size() > 1) {
		if (dirs[dirs.size()-2] == "packages"
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
		if (not *swtLsFiles) {
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

		if (*swtDocu) {
			commands::docu();
		} else if (*swtTest) {
			commands::test();
		} else if (*swtClean) {
			commands::clean();
		} else if (*swtCleanAll) {
			commands::cleanAll();
		} else if (optClone->size() == 2) {
			commands::clone((*optClone)[0], (*optClone)[1] + "/");
		} else if (optClone->size() == 1) {
			std::string url  = (*optClone)[0];
			std::string path = (*optClone)[0];
			if (utils::isEndingWith(path, ".git")) {
				for (int i {0}; i<4; ++i) path.pop_back();
			}
			auto l = utils::explode(path, "/");
			path = l[l.size()-1];
			commands::clone(url, path + "/");
		} else if (optGit->size() > 0) {
			commands::git(*optGit);
		} else if (*swtEclipse) {
			commands::eclipse();
		} else if (*swtPull) {
			commands::pull();
		} else if (*swtPush) {
			commands::push();
		} else if (*swtRelPath) {
			std::cout<<relPath<<std::endl;
		} else if (*swtInstall) {
			commands::install();
		} else if (*swtQuickFix or *swtQF) {
			commands::quickFix();
		} else if (*swtStatus) {
			commands::status();
		} else if (*optFlavor != "") {
			commands::status(*optFlavor);
		} else if (*swtLsFiles) {
			return commands::listFiles(relPath + "/");
		} else if (*swtToolchains) {
			commands::toolchains();
		} else if (*optToolchain != "") {
			commands::toolchain(*optToolchain);
		} else if (*swtClang) {
			commands::clang();
		} else {
			commands::build(*optBuild, *swtVerbose, *swtNoConsole);
		}
	} catch(std::runtime_error e) {
		std::cerr<<"exception(runtime_error): " << e.what() << std::endl;
	} catch(std::exception const& e) {
		std::cerr << "exception(exception): " << e.what() << std::endl;
	} catch(std::string const& s) {
		std::cerr << "exception(string): " << s << std::endl;
	} catch(char const* s) {
		std::cerr << "exception(char): " << s << std::endl;
	}
	return 0;
}
