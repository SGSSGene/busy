#include "utils.h"

#include <stdexcept>

namespace commonOptions {

auto splitPath(std::string _str) -> std::vector<std::string> {
	if (_str.size() == 0) return {};
	if (_str[0] == '.') _str = _str.substr(1);
	if (_str.size() == 0) return {};

	std::vector<std::string> retList;
	auto pos = _str.find_first_of('.');
	while (pos != std::string::npos) {
		if (pos == 0) throw std::runtime_error("option can't contain two points without any other character in between");
		retList.push_back(_str.substr(0, pos));
		_str = _str.substr(pos+1);
		pos = _str.find_first_of('.');
	}
	retList.push_back(_str);
	return retList;
}
}
