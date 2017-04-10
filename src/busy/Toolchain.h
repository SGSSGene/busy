#pragma once

#include <string>
#include <vector>

#include <busyConfig/Toolchain.h>

namespace busy {

struct Toolchain {
	using Command = busyConfig::Toolchain::Command;

	std::string version;
	Command cCompiler;
	Command cppCompiler;
	Command linkExecutable;
	Command archivist;

	auto operator=(Toolchain const&) -> Toolchain& = default;
	auto operator=(busyConfig::Toolchain const& _other) -> Toolchain {
		cCompiler      = _other.cCompiler;
		cppCompiler    = _other.cppCompiler;
		linkExecutable = _other.linkExecutable;
		archivist      = _other.archivist;

		return *this;
	}

	auto getAdditionalInfo() const -> std::string {
		std::vector<std::string> infos;
		if (version != "") {
			infos.push_back(version);
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
