
#include <commonOptions/commonOptions.h>
#include "commands/commands.h"

namespace {
	auto swtDocu       = commonOptions::make_switch("docu",       "Generates html docu");
	auto swtClang      = commonOptions::make_switch("clang",      "Generates a .clang_complete file");
	auto swtHelp       = commonOptions::make_switch("help",       "Shows some inforamation about this program");
	auto swtInstall    = commonOptions::make_switch("install",    "Installs the script to the current target");
	auto swtLsFiles    = commonOptions::make_switch("ls-files",   "Print all files of these repositories");
	auto swtPull       = commonOptions::make_switch("pull",       "Execute pull on all git repositories");
	auto swtPush       = commonOptions::make_switch("push",       "Execute push on all git repositories");
	auto swtQuickFix   = commonOptions::make_switch("quickfix",   "Quickfixes aBuild.json");
	auto swtQF         = commonOptions::make_switch("qf",         "Quickfixes aBuild.json");
	auto swtStatus     = commonOptions::make_switch("status",     "Shows current status of git repositories");
	auto swtTest       = commonOptions::make_switch("test",       "Run all unittests");
	auto swtToolchains = commonOptions::make_switch("toolchains", "Shows available toolchain");
	auto swtVerbose    = commonOptions::make_switch("verbose",    "Shows more information while running");

	auto optClone      = commonOptions::make_multi_option("clone", {}, "clones given git repository");
	auto optFlavor     = commonOptions::make_option("flavor",    "", "Changes current flavor");
	auto optToolchain  = commonOptions::make_option("toolchain", "", "Changes current toolchain");

}

using namespace aBuild;

using Action = std::function<void()>;

int main(int argc, char** argv) {
	if (not commonOptions::parse(argc, argv)) {
		commonOptions::print();
		return 0;
	} else if (*swtHelp) {
		commonOptions::print();
		return 0;
	}

	try {
		if (*swtDocu) {
			commands::docu();
		} else if (*swtTest) {
			commands::build(*swtVerbose);
			commands::test();
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
		} else if (*swtPull) {
			commands::pull();
		} else if (*swtPush) {
			commands::push();
		} else if (*swtInstall) {
			commands::install();
		} else if (*swtQuickFix or *swtQF) {
			commands::quickFix();
		} else if (*swtStatus) {
			commands::status();
		} else if (*optFlavor != "") {
			commands::status(*optFlavor);
		} else if (*swtLsFiles) {
			return commands::listFiles();
		} else if (*swtToolchains) {
			commands::toolchains();
		} else if (*optToolchain != "") {
			commands::toolchain(*optToolchain);
		} else if (*swtClang) {
			commands::clang();
		} else {
			commands::build(*swtVerbose);
		}
	} catch(std::exception const& e) {
		std::cerr<<"exception(exception): "<<e.what()<<std::endl;
	} catch(std::string const& s) {
		std::cerr<<"exception(string): "<<s<<std::endl;
	} catch(char const* s) {
		std::cerr<<"exception(char): "<<s<<std::endl;
	}
	return 0;
}
