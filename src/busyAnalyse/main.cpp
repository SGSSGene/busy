#include "FileCache.h"
#include "Package.h"
#include "Queue.h"

#include <process/Process.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

auto exceptionToString(std::exception const& e, int level = 0) -> std::string {
	std::string ret = std::string(level, ' ') + e.what();
	try {
		std::rethrow_if_nested(e);
	} catch(const std::exception& nested) {
		ret += "\n" + exceptionToString(nested, level+1);
	} catch(...) {
		ret += "\nprintable exception";
	}
	return ret;
}

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

struct Project {
	busy::analyse::Project const* project;

	std::set<Project const*> dependencies;
	std::set<Project const*> dependOnThis;
};

auto createProjects(std::vector<busy::analyse::Project> const& _projects) {
	auto projects = std::vector<Project>{};
	for (auto const& p : _projects) {
		projects.push_back({&p});
	}

	for (auto& p : projects) {
		auto deps = findDependentProjects(*p.project, _projects);

		for (auto& p2 : projects) {
			if (&p == &p2) continue;

			if (deps.count(p2.project) > 0) {
				p.dependencies.insert(&p2);
				p2.dependOnThis.insert(&p);
			}
		}
	}

	return projects;
}

auto readFullFile(std::filesystem::path const& file) {
	auto ifs = std::ifstream{file, std::ios::binary};
	ifs.seekg(0, std::ios::end);
	auto buffer = std::vector<std::byte>(ifs.tellg());
	ifs.seekg(0, std::ios::beg);
	ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
	return buffer;
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
			auto buffer = readFullFile(".filecache");
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

	for (auto const& p : projects2) {
		auto const& project = *p.project;
		std::cout << "\n";

		std::cout << "  - project-name: " << project.getName() << "\n";
		std::cout << "    path: " << project.getPath() << "\n";
		if (not project.getSystemLibraries().empty()) {
			std::cout << "    systemLibraries:\n";
			for (auto const& l : project.getSystemLibraries()) {
				std::cout << "    - " << l << "\n";
			}
		}
		if (not p.dependencies.empty()) {
			std::cout << "    dependencies:\n";
			for (auto const& d : p.dependencies) {
				std::cout << "    - " << d->project->getName() << " (" << d->project->getPath() << ")\n";
			}
		}

	}

	{
		using Q = Queue<busy::analyse::Project const, busy::analyse::File const>;
		auto nodes = Q::Nodes{};
		auto edges = Q::Edges{};

		for (auto& p : projects2) {
			nodes.push_back(p.project);
			for (auto& [fileType, list] : p.project->getFiles()) {
				for (auto& file : list) {
					nodes.emplace_back(&file);
					edges.emplace_back(Q::Edge{&file, p.project});
				}
			}
			for (auto& d : p.dependencies) {
				edges.emplace_back(Q::Edge{d->project, p.project});
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
			params.emplace_back(canonical(rootPath));
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
		auto linkLibrary = [&](busy::analyse::Project const& project) -> std::vector<std::string> {

			if (project.isHeaderOnly()) {
				return {};
			}

			auto target = (std::filesystem::path{"lib"} / project.getName()).replace_extension(".a");

			auto params = std::vector<std::string>{};
			params.emplace_back("./toolchainCall.sh");
			params.emplace_back("link");
			params.emplace_back("static_library");
			params.emplace_back(canonical(rootPath));

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

			return params;
		};
		auto linkExecutable = [&](busy::analyse::Project const& project) {
			auto target = std::filesystem::path{"bin"} / project.getName();

			auto params = std::vector<std::string>{};
			params.emplace_back("./toolchainCall.sh");
			params.emplace_back("link");
			params.emplace_back("executable");
			params.emplace_back(canonical(rootPath));

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
					//std::cout << "dep: " << project.getName() << "\n";
					if (not project.isHeaderOnly()) {
						auto target = (std::filesystem::path{"lib"} / project.getName()).replace_extension(".a");
						params.emplace_back(target);
					}
					addSystemLibraries(project);
				}
			});

			for (auto const& l : systemLibraries) {
				params.emplace_back("-l"+l);
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
							for (auto const& p : projects2) {
								if (p.project == &x) {
									return p.dependOnThis.empty();
								}
							}
							assert(false);
						}();
						if (executable) {
							return linkExecutable(x);
						} else {
							return linkLibrary(x);
						}
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
		std::cerr << "exception: " << exceptionToString(e) << "\n";
	}
	return EXIT_FAILURE;
}
