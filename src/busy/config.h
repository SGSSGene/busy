#pragma once

namespace busy {

//!TODO should follow XDG variables (see cppman) and may be be not hard coded?
inline static auto global_sharedPath = std::filesystem::path{"/usr/share/busy"};
inline static auto user_sharedPath   = []() {
	return std::filesystem::path{getenv("HOME")} / ".config/busy";
}();

inline static auto global_toolchainDir   = std::filesystem::path{"toolchains.d"};
inline static auto global_busyConfigFile = std::filesystem::path{".busy.yaml"};


struct Config {
	struct {
		std::string name {"default"};
		std::string call {"toolchainCall.sh"};
	} toolchain;

	std::string rootDir {};


	template <typename Node>
	void serialize(Node& node) {
		node["toolchain_name"] % toolchain.name;
		node["toolchain_call"] % toolchain.call;
		node["rootDir"]        % rootDir;
	}
};

}
