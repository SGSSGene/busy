#include "File.h"

#include <cstring>
#include <busyUtils/busyUtils.h>
#include <fstream>

namespace busy {
namespace analyse {

namespace {
	void skipAllWhiteSpaces(char const*& str) {
		while (*str != '\0' and (*str == '\t' or *str == ' ')) ++str;
	}
	bool checkIfMakroEndif(char const* str) {
		skipAllWhiteSpaces(str);
		return strncmp(str, "#endif", 6) == 0;
	}
	bool checkIfMakroIfAndBusy(char const* str) {
		skipAllWhiteSpaces(str);
		if (strncmp(str, "#if", 3) != 0) {
			return false;
		}
		str += 3;
		skipAllWhiteSpaces(str);
		return strncmp(str, "BUSY_", 5) == 0;
	}
	bool checkIfMakroSystemInclude(char const* str) {
		skipAllWhiteSpaces(str);
		if (strncmp(str, "#include", 8) != 0) {
			return false;
		}
		str += 8;
		skipAllWhiteSpaces(str);
		return *str == '<';
	}
}

auto File::readFileAsStr(std::string const& _file) const -> std::string {
	auto ifs = std::ifstream(_file);
	ifs.seekg(0, std::ios::end);
	auto size = ifs.tellg();
	auto buffer = std::string(size, ' ');
	ifs.seekg(0);
	ifs.read(&buffer[0], size);
	return buffer;
}

void File::readFile(std::string const& _file) {
	std::ifstream ifs(_file);
	std::string line;

	std::set<std::string> dependenciesAsString;

	bool optionalSection = false;

	while (std::getline(ifs, line)) {
		// check if it is '#endif'
		if (checkIfMakroEndif(line.c_str())) {
			optionalSection = false;
		} else if (checkIfMakroIfAndBusy(line.c_str())) {
			optionalSection = true;
		} else if (checkIfMakroSystemInclude(line.c_str())) {
			auto parts = utils::explode(line, std::vector<std::string>{" ", "\t"});

			std::string includeFile;
			if (parts.size() == 0) continue;
			if (parts.size() == 1) {
				includeFile = parts[0];
			} else {
				includeFile = parts[1];
			}

			auto pos1 = includeFile.find("<")+1;
			auto pos2 = includeFile.find(">")-pos1;

			auto file = includeFile.substr(pos1, pos2);

			if (optionalSection) {
				mIncludesOptional.insert(file);
				mIncludes.erase(file);
			} else if (mIncludes.count(file) == 0) {
				mIncludes.insert(file);
			}
		}
	}

}

}
}
