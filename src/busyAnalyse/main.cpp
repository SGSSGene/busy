#include "FileCache.h"
#include "Package.h"
#include "Queue.h"
#include "utils/utils.h"

#include <process/Process.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>

auto findDependentProjects(busy::analyse::Project const& _project, std::vector<busy::analyse::Project> const& _projects) {
	auto ret = std::set<busy::analyse::Project const*>{};
	auto _allIncludes = _project.getIncludes();
	for (auto const& project : _projects) {
		//!TODO this should be a list of all files
		auto files = project.getFiles().at(busy::analyse::FileType::H);
		for (auto const& file : files) {
			// check if is includable by default path
			{
				auto path = project.getName() / relative(file.getPath(), project.getPath());
				if (_allIncludes.count(path)) {
					ret.emplace(&project);
				}
			}
			// check if it is includable by legacy include path
			{
				for (auto const& p : project.getLegacyIncludePaths()) {
					auto path = relative(file.getPath(), p);
					if (_allIncludes.count(path)) {
						ret.emplace(&project);
					}
				}
			}
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

	auto projects = busy::analyse::readPackage(rootPath, ".");
	for (auto const& p : projects) {
		std::cout << p.getName() << " (" << p.getPath() << ")\n";
	}
	auto groupedProjects = std::map<std::string, std::vector<busy::analyse::Project const*>>{};
	for (auto const& p : projects) {
		groupedProjects[p.getName()].emplace_back(&p);
	}

	// check consistency of packages
	std::cout << "check consistency\n";

	for (auto const& [name, list] : groupedProjects) {
		std::cout << "name: " << name << "\n";
		for (auto const& p : list) {
			std::cout << "  - " << p->getName() << "\n";
		}
		//!TODO this can be done much more efficient
		if (size(list) > 1) {
			for (auto const& p1 : list) {
				for (auto const& p2 : list) {
					if (p1 == p2) continue;
					if (not p1->isEquivalent(*p2)) {
						throw std::runtime_error("two projects don't seem to be the same: " + p1->getName());
					} else {
						std::cout << p1->getPath() << " and " << p2->getPath() << " are the same\n";
					}
				}
			}
		}

	}

	auto projects2 = createProjects(projects);

	for (auto const& [i_project, dep] : projects2) {
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

	}

	{
		using Q = Queue<busy::analyse::Project const, busy::analyse::File const>;
		auto nodes = Q::Nodes{};
		auto edges = Q::Edges{};

		for (auto& [project, dep] : projects2) {
			nodes.push_back(project);
			for (auto& [fileType, list] : project->getFiles()) {
				for (auto& file : list) {
					nodes.emplace_back(&file);
					edges.emplace_back(Q::Edge{&file, project});
				}
			}
			for (auto& d : std::get<0>(dep)) {
				edges.emplace_back(Q::Edge{d, project});
			}
		}
		auto queue = Q{nodes, edges};

		struct SetupCompileFile {
			std::filesystem::path in;
			std::filesystem::path out;
			std::vector<std::filesystem::path> projectIncludes;
			std::vector<std::filesystem::path> systemIncludes;
		};

		auto compileFileSetup = [&](busy::analyse::File const& file) -> std::optional<SetupCompileFile> {
			auto ext = file.getPath().extension();
			if (ext != ".cpp" and ext != ".c") {
				return std::nullopt;
			}

			auto outFile = file.getPath().lexically_normal().replace_extension(".o");
			auto inFile  = file.getPath();

			auto& project = queue.find_outgoing<busy::analyse::Project const>(&file);

			auto projectIncludes = std::vector<std::filesystem::path>{};
			projectIncludes.emplace_back(project.getPath());
			projectIncludes.emplace_back(project.getPath().parent_path());
			projectIncludes.emplace_back(project.getPath().parent_path().parent_path() / "include");


			auto systemIncludes = std::vector<std::filesystem::path>{};
			queue.visit_incoming(&project, [&](auto& x) {
				using X = std::decay_t<decltype(x)>;
				if constexpr (std::is_same_v<X, busy::analyse::Project>) {
					systemIncludes.emplace_back(x.getPath().parent_path());
					for (auto const& i : x.getLegacyIncludePaths()) {
						systemIncludes.emplace_back(i);
					}
				}
			});

			return SetupCompileFile{inFile, outFile, projectIncludes, systemIncludes};
		};
		auto compileFile = [&](busy::analyse::File const& file) -> std::vector<std::string> {
			auto result = compileFileSetup(file);
			if (not result) {
				return {};
			}
			auto params = std::vector<std::string>{};

			params.emplace_back("./toolchainCall.sh");
			params.emplace_back("compile");
			params.emplace_back(result->in);
			params.emplace_back("obj" / result->out);
			params.emplace_back("-I");
			for (auto const& p : result->projectIncludes) {
				params.emplace_back(p.string());
			}
			params.emplace_back("-isystem");
			for (auto const& p : result->systemIncludes) {
				params.emplace_back(p.string());
			}
			return params;
		};

		auto createLinkTarget = [&](busy::analyse::Project const& project, std::string action, std::string const& target) {
			auto params = std::vector<std::string>{};
			params.emplace_back("./toolchainCall.sh");
			params.emplace_back("link");
			params.emplace_back(action);

			params.emplace_back(target);

			params.emplace_back("-i");
			for (auto file : queue.find_incoming<busy::analyse::File>(&project)) {
				auto ext = file->getPath().extension();
				if (ext == ".cpp" or ext == ".c") {
					auto objPath = "obj" / file->getPath();
					objPath.replace_extension(".o");
					params.emplace_back(objPath);
				}
			}

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
					if (not project.isHeaderOnly()) {
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

		while (not queue.empty()) {
			queue.dispatch([&](auto& x) {
				auto params = [&]() -> std::vector<std::string> {
					using X = std::decay_t<decltype(x)>;
					if constexpr (std::is_same_v<X, busy::analyse::File>) {
						return compileFile(x);
					} else if constexpr (std::is_same_v<X, busy::analyse::Project>) {
						auto executable = [&]() {
							for (auto const& [key, dep] : projects2) {
								if (key == &x) {
									return std::get<1>(dep).empty();
								}
							}
							assert(false);
						}();
						auto type       = std::string{executable?"executable":"static_library"};
						auto targetName = [&]() -> std::filesystem::path {
							if (executable) return std::filesystem::path{"bin"} / x.getName();
							return (std::filesystem::path{"lib"} / x.getName()).replace_extension(".a");
						}();

						return createLinkTarget(x, type, targetName);
					}
					return {};
				}();

				if (not params.empty()) {
					/*auto p = std::string{};
					for (auto const& s : params) {
						p += s + " ";
					}
					std::cout << "should run: " <<  p << "\n";*/
					auto p = process::Process{params};
					if (p.getStatus() != 0 or true) {
						std::stringstream ss;
						for (auto const& p : params) {
							ss << p << " ";
						}
						std::cout << ss.str() << "\n";

						std::cout << p.cout() << "\n";
						std::cerr << p.cerr() << "\n";
						if (p.getStatus() != 0) {
							std::cout << "error exit\n";
							exit(1);
						}
					}
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

int main(int argc, char const** argv) {
	try {
		auto args = std::vector<std::string_view>(argc);
		for (int i{0}; i<argc; ++i) {
			args[i] = argv[i];
		}
		app(args);
		return EXIT_SUCCESS;
	} catch (std::exception const& e) {
		std::cerr << "exception: " << busy::utils::exceptionToString(e, 0) << "\n";
	}
	return EXIT_FAILURE;
}
