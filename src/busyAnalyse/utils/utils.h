#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace busy::utils {
	auto explode(std::string const& _str, std::vector<char> const& _dels) -> std::vector<std::string>;

	void safeFileWrite(std::filesystem::path _dest, std::filesystem::path _src);
}
