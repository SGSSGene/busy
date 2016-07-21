#pragma once

#include <serializer/serializer.h>

namespace busy {

class System {
private:
	std::set<std::string> systems;
	std::string type;
	std::string url;
public:
	template<typename Node>
	void serialize(Node& node) {
		node["systems"] % systems;
		node["type"]    % type;
		node["url"]     % url;
	}

	auto getSystems() const -> std::set<std::string> const& {
		return systems;
	}
	auto getType() const -> std::string const& {
		return type;
	}
	auto getURL() const -> std::string const& {
		return url;
	}
	bool isInstalled() const;
	void install() const;
};


class Installation {
private:
	std::string name;
	std::vector<System> systems;
public:
	template<typename Node>
	void serialize(Node& node) {
		node["name"]    % name;
		node["systems"] % systems;
	}

	auto getName() const -> std::string const& {
		return name;
	}
	auto getSystems() const -> std::vector<System> const& {
		return systems;
	}
	bool isInstalled() const;
	void install() const;
};

}
