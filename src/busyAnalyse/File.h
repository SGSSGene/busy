#pragma once

#include "FileCache.h"

#include <array>
#include <set>
#include <string>


namespace busy::analyse {

/**
 *
 * searching for includes using <> not ""
 */
class File {
public:
	using Hash = std::array<unsigned char, picosha2::k_digest_size>;
private:
	std::filesystem::path mPath;
	std::filesystem::path mFlatPath; // !maybe this should be removed?

	std::set<std::filesystem::path> mIncludes;
	std::set<std::filesystem::path> mIncludesOptional;

	Hash mHash;

public:
	File(std::filesystem::path _path, std::filesystem::path _flatPath)
		: mPath     { std::move(_path) }
		, mFlatPath { std::move(_flatPath) }
	{
		readFile(mPath);
	}

	auto const& getPath() const {
		return mPath;
	}

	auto const& getFlatPath() const {
		return mFlatPath;
	}

	auto const& getIncludes() const {
		return mIncludes;
	}

	auto const& getIncludesOptional() const {
		return mIncludesOptional;
	}
	auto const& getHash() const {
		return mHash;
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
		auto info1 = getFileCache().getFileCache(mPath);
		auto info2 = getFileCache().getFileCache(_other.mPath);
		return info1.hash == info2.hash;
	}


private:
	void readFile(std::filesystem::path const& _file);
};

}
