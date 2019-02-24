#include "utils.h"

#include <algorithm>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace busy::utils {

auto explode(std::string const& _str, std::vector<char> const& _dels) -> std::vector<std::string> {
	auto retList = std::vector<std::string>{};
	auto buffer = std::string{};
	for (auto c : _str) {
		auto has_c = any_of(begin(_dels), end(_dels), [=](char _c) { return _c == c; });
		if (not has_c) {
			buffer += c;
		} else if (has_c and not buffer.empty()) {
			retList.emplace_back(std::move(buffer));
			buffer = "";
		}
	}
	if (not buffer.empty()) {
		retList.emplace_back(std::move(buffer));
	}
	return retList;
}

void safeFileWrite(std::filesystem::path _dest, std::filesystem::path _src) {
	int fd = ::open(_src.string().c_str(), O_APPEND);
	::fsync(fd);
	::close(fd);
	::rename(_src.string().c_str(), _dest.string().c_str());
}
}
