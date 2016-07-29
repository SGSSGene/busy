#pragma once

#include <set>
#include <string>

namespace busy {
	struct FileStat {
		struct {
			int64_t lastChange;
			std::set<std::string> dependenciesAsString;

			template <typename Node>
			void serialize(Node& node) {
				node["lastChange"]           % lastChange;
				node["dependenciesAsString"] % dependenciesAsString;
			}
		} mFileDiscovery;

		struct {
			int64_t lastChange;
			template <typename Node>
			void serialize(Node& node) {
				node["lastChange"] % lastChange;
			}
		} mFileCompile;

		FileStat() {
			mFileDiscovery.lastChange = 0;
		}

		template <typename Node>
		void serialize(Node& node) {
			node["fileDiscovery"] % mFileDiscovery;
			node["fileCompile"]   % mFileCompile;
		}
	};
}
