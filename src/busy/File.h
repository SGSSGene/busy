#pragma once

#include "FileCache.h"

#include <array>
#include <set>
#include <string>


namespace busy {

/**
 *
 * searching for includes using <> (includes using "" are being ignored)
 */
class File {
private:
	std::filesystem::path mRoot;
	std::filesystem::path mPath;

	std::set<std::filesystem::path> mIncludes;
public:

	File(std::filesystem::path const& _root, std::filesystem::path _path)
		: mRoot     { _root }
		, mPath     { std::move(_path) }
	{
		readFile(_root / mPath);
	}

	auto const& getPath() const {
		return mPath;
	}

	auto const& getIncludes() const {
		return mIncludes;
	}

	auto getHash() const -> std::string;

	auto isEquivalent(File const& _other) const -> bool {
		//!TODO needs to compare name
		if (mIncludes != _other.mIncludes) {
			return false;
		}
		auto h1 = getFileCache().getHash(mRoot / mPath);
		auto h2 = getFileCache().getHash(_other.mRoot / _other.mPath);
		return h1 == h2;
	}


private:
	void readFile(std::filesystem::path const& _file);
};

}
