#include "FileCache.h"
#include "Package.h"
#include "Queue.h"
#include "utils/utils.h"

#include <process/Process.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace busy {
namespace analyse {

// check if this _project is included by _allIncludes
auto isDependentProject(std::set<std::filesystem::path> const& _allIncludes, busy::analyse::Project const& _project) -> bool {

	auto files = _project.getFiles();
	for (auto const& file : files) {
		// check if is includable by default path
		{
			auto path = _project.getName() / relative(file.getPath(), _project.getPath());
			if (_allIncludes.count(path)) {
				return true;
			}
		}
		// check if it is includable by legacy include path
		{
			for (auto const& p : _project.getLegacyIncludePaths()) {
				auto path = relative(file.getPath(), p);
				if (_allIncludes.count(path)) {
					return true;
				}
			}
		}
	}
	return false;
}

auto findDependentProjects(busy::analyse::Project const& _project, std::vector<busy::analyse::Project> const& _projects) {
	auto ret = std::set<busy::analyse::Project const*>{};
	auto _allIncludes = _project.getIncludes();

	for (auto const& project : _projects) {
		if (isDependentProject(_allIncludes, project)) {
			ret.emplace(&project);
		}
	}
	return ret;
}

auto createProjects(std::vector<busy::analyse::Project> const& _projects) {
	using Project = busy::analyse::Project;

	auto ret = std::map<Project const*, std::tuple<std::set<Project const*>, std::set<Project const*>>>{};

	for (auto const& p : _projects) {
		ret[&p];
		auto deps = findDependentProjects(p, _projects);
		for (auto const& d : deps) {
			if (d == &p) continue;
			std::get<0>(ret[&p]).insert(d);
			std::get<1>(ret[d]).insert(&p);
		}
	}

	return ret;
}

void checkConsistency(std::vector<busy::analyse::Project> const& _projects) {
	auto groupedProjects = std::map<std::string, std::vector<busy::analyse::Project const*>>{};
	for (auto const& p : _projects) {
		groupedProjects[p.getName()].emplace_back(&p);
	}


	for (auto const& [name, list] : groupedProjects) {
		//!TODO this can be done much more efficient
		if (size(list) > 1) {
			for (auto const& p1 : list) {
				for (auto const& p2 : list) {
					if (p1 == p2) continue;
					if (not p1->isEquivalent(*p2)) {
						throw std::runtime_error("two projects don't seem to be the same: " + p1->getName());
					}
				}
			}
		}
	}
}

void printProjects(std::map<Project const*, std::tuple<std::set<Project const*>, std::set<Project const*>>> const& _projects) {
	for (auto const& [i_project, dep] : _projects) {
		auto const& project = *i_project;
		auto const& dependencies = std::get<0>(dep);
		std::cout << "\n";

		std::cout << "  - project-name: " << project.getName() << "\n";
		std::cout << "    path: " << project.getPath() << "\n";
		if (not project.getSystemLibraries().empty()) {
			std::cout << "    systemLibraries:\n";
			for (auto const& l : project.getSystemLibraries()) {
				std::cout << "    - " << l << "\n";
			}
		}
		if (not dependencies.empty()) {
			std::cout << "    dependencies:\n";
			for (auto const& d : dependencies) {
				std::cout << "    - " << d->getName() << " (" << d->getPath() << ")\n";
			}
		}
		if (not project.getLegacyIncludePaths().empty()) {
			std::cout << "    includePaths:\n";
			for (auto const& p : project.getLegacyIncludePaths()) {
				std::cout << "      - " << p << "\n";
			}
		}
	}
}

//!TODO should follow XDG variables (see cppman) and may be be not hard coded?
static auto global_sharedPath = std::filesystem::path{"/usr/share/busy"};
static auto user_sharedPath   = []() {
	return std::filesystem::path{getenv("HOME")} / ".config/busy";
}();

auto searchForToolchains(std::filesystem::path _path) {
	auto retList = std::vector<std::tuple<std::string, std::filesystem::path>>{};

	_path /= "toolchains.d";
	if (exists(_path)) {
		for (auto f : std::filesystem::directory_iterator{_path}) {
			auto params = std::vector<std::string>{f.path(), "info"};
			auto p = process::Process{params};
			if (p.getStatus() == 0) {
				auto node = YAML::Load(p.cout());
				if (node["toolchains"].IsSequence()) {
					for (auto const& n : node["toolchains"]) {
						retList.emplace_back(n["name"].as<std::string>(), f.path());
					}
				}
			}
		}
	}


	return retList;
}
auto searchForToolchains(std::vector<std::filesystem::path> const& _paths) {
	auto retList = std::map<std::string, std::filesystem::path>{};
	for (auto const& p : _paths) {
		for (auto const& [name, path] : searchForToolchains(p)) {
			retList[name] = path;
		}
	}
	return retList;
}

void app(std::vector<std::string_view> args) {
	if (size(args) <= 1) {
		throw std::runtime_error("please give path of busy.yaml");
	}

	auto rootPath = relative(std::filesystem::path(args[1]));
	if (rootPath == ".") {
		throw std::runtime_error("can't build in source, please create a `build` directory");
	}

	getFileCache() = [&]() {
		if (std::filesystem::exists(".filecache")) {
			auto buffer = busy::utils::readFullFile(".filecache");
			return fon::binary::deserialize<FileCache>(buffer);
		} else if (std::filesystem::exists(".filecache.yaml")) {
			return fon::yaml::deserialize<FileCache>(YAML::LoadFile(".filecache.yaml"));
		}
		return FileCache{};
	}();

	auto [projects, packages] = busy::analyse::readPackage(rootPath, ".");

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	if (args.size() > 2 and args[2] == "ls-toolchains") {
		for (auto [name, path]  : searchForToolchains(packages)) {
			std::cout << "  - " << name << " (" << path << ")\n";
		}
		return;
	}

	auto toolchainCall = std::string{"./toolchainCall.sh"};

	if (args.size() > 3 and args[2] == "toolchain") {
		auto toolchains = searchForToolchains(packages);
		auto iter = toolchains.find(std::string{args[3]});
		if (iter == toolchains.end()) {
			throw std::runtime_error("could not find toolchain \"" + std::string{args[3]} + "\"");
		}

		toolchainCall = iter->second;
		std::cout << "using toolchain: " << args[3] << " calling " << toolchainCall << "\n";
	}

//	for (auto const& p : projects) {
//		std::cout << p.getName() << " (" << p.getPath() << ")\n";
//	}

	// check consistency of packages
	std::cout << "checking consistency...\n";

	checkConsistency(projects);

	auto projects_with_deps = createProjects(projects);
	printProjects(projects_with_deps);

	{
		using Q = Queue<busy::analyse::Project const, busy::analyse::File const>;

		auto nodes = Q::Nodes{};
		auto edges = Q::Edges{};
		auto colors = std::map<Q::Node, int>{};

		for (auto& [project, dep] : projects_with_deps) {
			nodes.push_back(project);
			for (auto& file : project->getFiles()) {
				nodes.emplace_back(&file);
				edges.emplace_back(Q::Edge{&file, project});
			}
			for (auto& d : std::get<0>(dep)) {
				edges.emplace_back(Q::Edge{d, project});
			}
		}
		auto queue = Q{nodes, edges};


		auto setupCompiling = [&](busy::analyse::File const& file) -> std::vector<std::string> {
			auto outFile = file.getPath().lexically_normal().replace_extension(".o");
			auto inFile  = file.getPath();

			auto params = std::vector<std::string>{};

			params.emplace_back(toolchainCall);
			params.emplace_back("compile");
			params.emplace_back(inFile);
			params.emplace_back("obj" / outFile);

			// add all include paths
			params.emplace_back("-I");

			auto& project = queue.find_outgoing<busy::analyse::Project const>(&file);

			params.emplace_back(project.getPath());
			for (auto const& p : project.getLegacyIncludePaths()) {
				params.emplace_back(p);
			}

			// add all system include paths
			params.emplace_back("-isystem");

			auto systemIncludes = std::vector<std::filesystem::path>{};
			queue.visit_incoming(&project, [&](auto& x) {
				using X = std::decay_t<decltype(x)>;
				if constexpr (std::is_same_v<X, busy::analyse::Project>) {
					params.emplace_back(x.getPath().parent_path());
					for (auto const& i : x.getLegacyIncludePaths()) {
						params.emplace_back(i);
					}
				}
			});

			return params;
		};

		auto setupLinking = [&](busy::analyse::Project const& project) {
			auto [action, target] = [&]() -> std::tuple<std::string, std::filesystem::path> {
				bool isExecutable = std::get<1>(projects_with_deps.at(&project)).empty();
				if (isExecutable) {
					return {"executable", std::filesystem::path{"bin"} / project.getName()};
				}
				return {"static_library", (std::filesystem::path{"lib"} / project.getName()).replace_extension(".a")};
			}();

			auto params = std::vector<std::string>{};
			params.emplace_back(toolchainCall);
			params.emplace_back("link");
			params.emplace_back(action);

			params.emplace_back(target);

			params.emplace_back("-i");
			for (auto file : queue.find_incoming<busy::analyse::File>(&project)) {
				if (colors.at(file) == 0) {
					auto objPath = "obj" / file->getPath();
					objPath.replace_extension(".o");
					params.emplace_back(objPath);
				}
			}

			params.emplace_back("-il");

			// add all legacy system libraries
			std::vector<std::string> systemLibraries;
			auto addSystemLibraries = [&](busy::analyse::Project const& project) {
				for (auto const& l : project.getSystemLibraries()) {
					auto iter = std::find(begin(systemLibraries), end(systemLibraries), l);
					if (iter != end(systemLibraries)) {
						systemLibraries.erase(iter);
					}
					systemLibraries.push_back(l);
				}
			};

			addSystemLibraries(project);

			queue.visit_incoming(&project, [&](auto& project) {
				using X = std::decay_t<decltype(project)>;
				if constexpr (std::is_same_v<X, busy::analyse::Project>) {
					if (colors.at(&project) == 0) {
						auto target = (std::filesystem::path{"lib"} / project.getName()).replace_extension(".a");
						params.emplace_back(target);
					}
					addSystemLibraries(project);
				}
			});

			params.emplace_back("-l");
			for (auto const& l : systemLibraries) {
				params.emplace_back(l);
			}

			return params;
		};
		auto setup = [&](auto & x) -> std::vector<std::string> {
			using X = std::decay_t<decltype(x)>;
			if constexpr (std::is_same_v<X, busy::analyse::File>) {
				return setupCompiling(x);
			} else if constexpr (std::is_same_v<X, busy::analyse::Project>) {
				return setupLinking(x);
			}
			assert(false);
		};



		while (not queue.empty()) {
			queue.dispatch([&](auto& x) {
				auto params = setup(x);
				/*auto p = std::string{};
				for (auto const& s : params) {
					p += s + " ";
				}
				std::cout << "should run: " <<  p << "\n";*/
				auto p = process::Process{params};
				if (p.getStatus() != 0 and p.getStatus() != 1 or true) {
					std::stringstream ss;
					for (auto const& p : params) {
						ss << p << " ";
					}
					std::cout << ss.str() << "\n";

					std::cout << p.cout() << "\n";
					std::cerr << p.cerr() << "\n";
					if (p.getStatus() != 0 and p.getStatus() != 1) {
						std::cout << "error exit\n";
						exit(1);
					}
				}
				if (p.getStatus() == 1) {
					colors[&x] = 1;
				} else {
					colors[&x] = 0;
				}
			});
		}
	}

	{ // write binary
	auto node = fon::binary::serialize(getFileCache());
	auto ofs = std::ofstream{".filecache", std::ios::binary};
	std::cout << "node: " << node.size() << "\n";
	ofs.write(reinterpret_cast<char const*>(node.data()), node.size());
	}
	if (false) { // write yaml
		YAML::Emitter out;
		out << fon::yaml::serialize(getFileCache());
		std::ofstream(".filecache.yaml") << out.c_str();
	}
}
}
}

int main(int argc, char const** argv) {
	try {
		auto args = std::vector<std::string_view>(argc);
		for (int i{0}; i<argc; ++i) {
			args[i] = argv[i];
		}
		busy::analyse::app(args);
		return EXIT_SUCCESS;
	} catch (std::exception const& e) {
		std::cerr << "exception: " << busy::utils::exceptionToString(e, 0) << "\n";
	}
	return EXIT_FAILURE;
}
