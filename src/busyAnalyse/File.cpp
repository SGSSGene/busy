#include "File.h"

#include <cstring>
#include <busyUtils/busyUtils.h>
#include <fstream>

namespace busy::analyse {

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

	auto computeHash(std::filesystem::path const& path) -> std::string {
		std::cout << "computing hash of: " << path << "\n";
		std::ifstream ifs(path, std::ios::binary);
		using Hash = std::array<unsigned char, picosha2::k_digest_size>;
		Hash hash;
		picosha2::hash256(ifs, hash.begin(), hash.end());
		auto b64 = base64_encode(&hash[0], picosha2::k_digest_size);
		return b64;
	}

}

namespace {
	auto readIncludes(std::filesystem::path const& _file) -> std::tuple<std::set<std::filesystem::path>, std::set<std::filesystem::path>>{
		std::ifstream ifs(_file);
		std::string line;


		std::set<std::string> dependenciesAsString;

		bool optionalSection = false;

		std::set<std::filesystem::path> resIncludes;
		std::set<std::filesystem::path> resIncludesOptional;

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
					resIncludesOptional.insert(std::filesystem::path(file));
					resIncludes.erase(file);
				} else if (resIncludes.count(file) == 0) {
					resIncludes.insert(file);
				}
			}
		}
		return {std::move(resIncludes), std::move(resIncludesOptional)};
	}
}

auto File::getHash() const -> std::string {

	if (getFileCache().hasTChange<std::string>(mPath)) {
		getFileCache().updateT<std::string>(mPath, computeHash(mPath));
	}
	return getFileCache().getT<std::string>(mPath);
}


void File::readFile(std::filesystem::path const& _file) {
	using AllIncludes = FileCache::AllIncludes;

	if (getFileCache().hasTChange<AllIncludes>(_file)) {
		getFileCache().updateT<AllIncludes>(_file, readIncludes(_file));
	}

	auto const& [incl, optIncl] = getFileCache().getT<AllIncludes>(_file);
	mIncludes = incl;
	mIncludesOptional = optIncl;
}

}
