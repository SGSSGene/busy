#include "selfTest.h"

namespace selfTest {
namespace {
auto explode(std::string const& _str, std::vector<std::string> const& _del) -> std::vector<std::string> {
	auto str = _str;
	auto retList = std::vector<std::string>{};
	while (str.length() > 0) {
		auto p = str.find(_del[0]);
		int delSize = _del[0].length();
		for (auto const& s : _del) {
			auto _p = str.find(s);
			if (_p == std::string::npos) continue;
			if (_p < p) {
				p = _p;
				delSize = s.length();
			}
		}
		auto subStr = str.substr(0, p);
		retList.push_back(subStr);
		if (p == std::string::npos) {
			return retList;
		}
		str.erase(0, p + delSize);
	}
	return retList;
}
}

auto SelfTest::addCase(std::string const& _name, std::string const& _params, int _status, std::string const& _stdcout, std::string const& _stdcerr) -> bool {
	auto p = explode(_params, {" "});
	testCase.emplace_back(_name, p, _status, _stdcout, _stdcerr);
	return true;
}


}

