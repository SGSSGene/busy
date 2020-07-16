#pragma once
#include "InteractiveProcess.h"

namespace process {

class ProcessPImpl;

class Process final {
private:
	std::unique_ptr<ProcessPImpl> pimpl;
public:
	Process(std::vector<std::string> const& prog);
	Process(std::vector<std::string> const& prog, std::string const& _cwd);

	~Process();
	Process(Process const&) = delete;
	Process(Process&&) = delete;
	auto operator=(Process const&) -> Process& = delete;
	auto operator=(Process&&) -> Process& = delete;

	[[nodiscard]] auto cout() const -> std::string const&;
	[[nodiscard]] auto cerr() const -> std::string const&;
	[[nodiscard]] auto getStatus() const -> int;
};

}
