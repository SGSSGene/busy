#pragma once

#include <set>
#include <string>

namespace busy {
namespace analyse {

/**
 *
 * searching for includes using <> not ""
 */
class File {
private:
	std::set<std::string> mIncludes;
	std::set<std::string> mIncludesOptional;

public:
	File(std::string const& _file) {
		readFile(_file);
	}

	auto getIncludes() const -> std::set<std::string> const& {
		return mIncludes;
	}

	auto getIncludesOptional() const -> std::set<std::string> const& {
		return mIncludesOptional;
	}

private:
	void readFile(std::string const& _file);
};
}
}
