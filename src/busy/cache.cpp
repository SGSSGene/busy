#include "cache.h"

#include "utils/utils.h"
#include "FileCache.h"

namespace busy {

void loadFileCache() {
	getFileCache() = [&]() {
		// load binary cache
		if (std::filesystem::exists(".filecache")) {
			auto buffer = busy::utils::readFullFile(".filecache");
			return fon::binary::deserialize<FileCache>(buffer);
		// load yaml cache
		} else if (std::filesystem::exists(".filecache.yaml")) {
			return fon::yaml::deserialize<FileCache>(YAML::LoadFile(".filecache.yaml"));
		}
		return FileCache{};
	}();
}
void saveFileCache() {
	// write binary cache
	{
		auto node = fon::binary::serialize(getFileCache());
		auto ofs = std::ofstream{".filecache", std::ios::binary};
		ofs.write(reinterpret_cast<char const*>(node.data()), node.size());
	}

	// write yaml cache
	if (false) {
		YAML::Emitter out;
		out << fon::yaml::serialize(getFileCache());
		std::ofstream(".filecache.yaml") << out.c_str();
	}
}

}
