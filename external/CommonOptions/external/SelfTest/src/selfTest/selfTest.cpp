#include "selfTest.h"

namespace selfTest {

static std::vector<std::string> explode(std::string const& _str, std::vector<std::string> const& _del) {
	auto str = _str;
	std::vector<std::string> retList;
	while (str.length() > 0) {
		auto p = str.find(_del[0]);
		int delSize = _del[0].length();
		for (auto const s : _del) {
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

bool SelfTest::addCase(std::string _name, std::string _params, int _status, std::string _stdcout, std::string _stdcerr) {
	auto p = explode(_params, {" "});
	testCase.emplace_back(_name, p, _status, _stdcout, _stdcerr);
	return true;
}


}

