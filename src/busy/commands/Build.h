#pragma once

#include "Workspace.h"

#include <busyUtils/busyUtils.h>
#include <functional>
#include <mutex>
#include <string>

namespace busy {
namespace commands {
namespace detail {

class Build {
private:
	Workspace& ws;
	std::string buildPath;
	std::string outPath;
	std::string rootProjectName;
	bool        console;

public:
	bool errorDetected {false};
	mutable std::mutex printMutex;


public:
	Build(Workspace& _ws, std::string _buildPath, std::string _outPath, std::string _rootProjectName, bool _console)
		: ws (_ws)
		, buildPath(std::move(_buildPath))
		, outPath(std::move(_outPath))
		, rootProjectName(std::move(_rootProjectName))
		, console(_console)
	{}


	void createAllNeededPaths() const;
	void createVersionFiles() const;

	auto getStatisticUpdateCallback() const -> std::function<void(int, int)>;

private:
	void updateStatisticUpdate(int done, int total) const;
};


}
}
}
