#pragma once

#include <busyConfig/busyConfig.h>
namespace busy {

class System {
private:
	std::set<std::string> systems;
	std::string type;
	std::string url;
public:
	System() {}
	System(busyConfig::InstallationSystem const& _system);

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
	Installation() {}
	Installation(busyConfig::Installation const& _installation);

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
