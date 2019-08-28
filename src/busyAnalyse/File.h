#pragma once

#include "FileCache.h"

#include <array>
#include <set>
#include <string>


namespace busy::analyse {

/**
 *
 * searching for includes using <> (includes using "" are being ignored)
 */
class File {
private:
	std::filesystem::path mRoot;
	std::filesystem::path mPath;

	std::set<std::filesystem::path> mIncludes;
	std::set<std::filesystem::path> mIncludesOptional;
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

	auto const& getIncludesOptional() const {
		return mIncludesOptional;
	}
	auto getHash() const -> std::string;

	auto isEquivalent(File const& _other) const -> bool {
		//!TODO needs to compare name
		if (mIncludes != _other.mIncludes) {
			return false;
		}
		if (mIncludesOptional != _other.mIncludesOptional) {
			return false;
		}
		auto info1 = getFileCache().getFileCache(mRoot / mPath);
		auto info2 = getFileCache().getFileCache(_other.mRoot / _other.mPath);
		return info1.hash == info2.hash;
	}


private:
	void readFile(std::filesystem::path const& _file);
};

}
