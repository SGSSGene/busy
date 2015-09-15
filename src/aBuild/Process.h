#include <memory>
#include <string>
#include <vector>


namespace utils {

class ProcessPImpl;

class Process final {
private:
	ProcessPImpl* pimpl;
public:
	Process(std::string const& s, std::vector<std::string> const& argv);

	~Process();
	Process(Process const&) = delete;
	Process(Process&&) = delete;
	Process& operator=(Process const&) = delete;
	Process& operator=(Process&&) = delete;

	auto cout() const -> std::string const&;
	auto cerr() const -> std::string const&;
	int  getStatus() const;
};


}
