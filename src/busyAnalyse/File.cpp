#include "File.h"

#include "utils/utils.h"

#include <cstring>
#include <fstream>

namespace busy::analyse {

namespace {

void skipAllWhiteSpaces(char const*& str) {
	while (*str != '\0' and (*str == '\t' or *str == ' ')) ++str;
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

			auto pos1 = includeFile.find("<") + 1;
			auto pos2 = includeFile.find(">") - pos1;

			auto file = includeFile.substr(pos1, pos2);

			resIncludes.insert(file);
		}
	}
	return resIncludes;
}
}

auto File::getHash() const -> std::string {

	if (getFileCache().hasTChange<std::string>(mPath)) {
		getFileCache().updateT<std::string>(mPath, computeHash(mPath));
	}
	return getFileCache().getT<std::string>(mPath);
}


void File::readFile(std::filesystem::path const& _file) {
	using Includes = std::set<std::filesystem::path>;

	if (getFileCache().hasTChange<Includes>(_file)) {
		getFileCache().updateT<Includes>(_file, readIncludes(_file));
	}

	mIncludes = getFileCache().getT<Includes>(_file);
}

}
