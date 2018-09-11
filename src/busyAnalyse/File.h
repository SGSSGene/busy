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
	std::string mPath;
	std::string mFlatPath; // !maybe this should be removed?

	std::set<std::string> mIncludes;
	std::set<std::string> mIncludesOptional;

public:
	File(std::string _path, std::string _flatPath)
		: mPath     { std::move(_path) }
		, mFlatPath { std::move(_flatPath) }
	{
		readFile(mPath);
	}

	auto getPath() const -> std::string const& {
		return mPath;
	}
	auto getFlatPath() const -> std::string const& {
		return mFlatPath;
	}

	auto getIncludes() const -> std::set<std::string> const& {
		return mIncludes;
	}

	auto getIncludesOptional() const -> std::set<std::string> const& {
		return mIncludesOptional;
	}

	auto isEquivalent(File const& _other) const -> bool {
		if (mFlatPath != _other.mFlatPath) {
			return false;
		}
		if (mIncludes != _other.mIncludes) {
			return false;
		}
		if (mIncludesOptional != _other.mIncludesOptional) {
			return false;
		}
		return true;
	}


private:
	void readFile(std::string const& _file);
};
}
}
