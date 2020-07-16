#include "File.h"

#include "utils/utils.h"

#include <cstring>
#include <fstream>

namespace busy {

namespace {

void skipAllWhiteSpaces(char const*& str) {
	while (*str != '\0' and (*str == '\t' or *str == ' ')) ++str;
}
auto checkIfMakroSystemInclude(char const* str) -> bool {
	skipAllWhiteSpaces(str);
	if (strncmp(str, "#include", 8) != 0) {
		return false;
	}
	str += 8;
	skipAllWhiteSpaces(str);
	return *str == '<';
}

auto readIncludes(std::filesystem::path const& _file) -> std::set<std::filesystem::path>{
	auto dependenciesAsString = std::set<std::string>{};

	auto resIncludes          = std::set<std::filesystem::path>{};

	auto ifs  = std::ifstream(_file);
	auto line = std::string{};
	while (std::getline(ifs, line)) {
		if (checkIfMakroSystemInclude(line.c_str())) {
			auto parts = utils::explode(line, {' ', '\t'});

			if (parts.size() == 0) continue;

			auto includeFile = [&]() -> std::string {
				if (parts.size() == 1) {
					return parts[0];
				}
				return parts[1];
			}();

			auto pos1 = includeFile.find('<') + 1;
			auto pos2 = includeFile.find('>') - pos1;

			auto file = includeFile.substr(pos1, pos2);

			resIncludes.insert(file);
		}
	}
	return resIncludes;
}
}

auto File::getHash() const -> std::string {
	return getFileCache().getHash(mPath);
}


void File::readFile(std::filesystem::path const& _file) {
	using Includes = std::set<std::filesystem::path>;
	mIncludes = getFileData().checkAndRetrieve<Includes>(_file, [&]() {
		return readIncludes(_file);
	});
}

}
