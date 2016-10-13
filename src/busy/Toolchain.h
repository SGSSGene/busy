#pragma once

#include <string>
#include <vector>

#include <busyConfig/Toolchain.h>

namespace busy {

struct Toolchain {
	struct Command {
		std::vector<std::string> command;
		std::vector<std::string> postOptions;
	};

	std::string version;
	bool        crossCompiler;
	Command cCompiler;
	Command cppCompiler;
	Command archivist;

	auto operator=(Toolchain const&) -> Toolchain& = default;
	auto operator=(busyConfig::Toolchain const& _other) -> Toolchain {
		version                 = _other.version;
		crossCompiler           = _other.crossCompiler;
		cCompiler.command       = _other.cCompiler.command;
		cCompiler.postOptions   = _other.cCompiler.postOptions;
		cppCompiler.command     = _other.cppCompiler.command;
		cppCompiler.postOptions = _other.cppCompiler.postOptions;
		archivist.command       = _other.archivist.command;
		archivist.postOptions   = _other.archivist.postOptions;

		return *this;
	}

	auto getAdditionalInfo() const -> std::string {
		std::vector<std::string> infos;
		if (version != "") {
			infos.push_back(version);
		}
		if (crossCompiler) {
			infos.push_back("cross compiler");
		}
		if (infos.empty()) return "";
		if (infos.size() == 1) return infos.front();

		std::string output = infos.front();
		for (auto iter = infos.begin() + 1; iter != infos.end(); ++iter) {
			output += ", " + *iter;
		}
		return output;
	}
};

}
