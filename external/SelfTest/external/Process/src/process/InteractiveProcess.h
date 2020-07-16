#pragma once

#include <memory>
#include <string>
#include <vector>


namespace process {

class InteractiveProcessPImpl;

class InteractiveProcess final {
private:
	std::unique_ptr<InteractiveProcessPImpl> pimpl;
public:
	InteractiveProcess(std::vector<std::string> const& prog);
	InteractiveProcess(std::vector<std::string> const& prog, std::string const& _cwd);

	InteractiveProcess(InteractiveProcess const&) = delete;
	InteractiveProcess(InteractiveProcess&&) = delete;
	auto operator=(InteractiveProcess const&) -> InteractiveProcess& = delete;
	auto operator=(InteractiveProcess&&) -> InteractiveProcess& = delete;
};




}
