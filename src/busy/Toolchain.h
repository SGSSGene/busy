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

	Command cCompiler;
	Command cppCompiler;
	Command archivist;

	auto operator=(Toolchain const&) -> Toolchain& = default;
	auto operator=(busyConfig::Toolchain const& _other) -> Toolchain {
		cCompiler.command       = _other.cCompiler.command;
		cCompiler.postOptions   = _other.cCompiler.postOptions;
		cppCompiler.command     = _other.cppCompiler.command;
		cppCompiler.postOptions = _other.cppCompiler.postOptions;
		archivist.command       = _other.archivist.command;
		archivist.postOptions   = _other.archivist.postOptions;

		return *this;
	}
};

}
