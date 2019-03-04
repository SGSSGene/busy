#include "FileCache.h"
#include "Package.h"
#include "Queue.h"

#include <process/Process.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
	auto isPrefix(std::filesystem::path const& _prefix, std::filesystem::path const& _path) -> bool {
		auto prefix = std::string{_prefix};
		auto path   = std::string{_path};
		if (prefix.size() > path.size()) return false;

		auto [iterL, iterR] = std::mismatch(begin(prefix), end(prefix), begin(path));
		return iterL == end(prefix);
	}
}

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

void printProject(std::string tabs, busy::analyse::Project const& project) {
	std::cout << tabs << " - name: " << project.getName() << "\n";
	std::cout << tabs << "   path: " << project.getPath() << "\n";
	std::cout << tabs << "   files:\n";
	for (auto const& [key, value] : project.getFiles()) {
		if (value.empty()) continue;
		auto keyToString = [](busy::analyse::FileType type) -> std::string {
			using FileType = busy::analyse::FileType;
			if (type == FileType::Cpp) {
				return "cpp";
			} else if (type == FileType::C) {
				return "c";
			} else {
				return "incl";
			}
		};
		std::cout << tabs << "     - " << keyToString(key) << ":\n";
		for (auto const& f : value) {
			std::cout << tabs << "       - " << f.getPath() << "\n";
		}
	}
	std::cout << tabs << "   includes:\n";

	std::set<std::string> allIncludes;
	for (auto const& [type, files] : project.getFiles()) {
		if (files.empty()) continue;
		for (auto const& f : files) {
			auto incl = f.getIncludes();
			allIncludes.insert(begin(incl), end(incl));
		}
	}
	for (auto const& i : allIncludes) {
		std::cout << tabs << "     - " << i << "\n";
	}
}

void printPackage(std::string tabs, busy::analyse::Package const& package) {
	std::cout << tabs << "- name: " << package.getName() << "\n";
	tabs += "  ";
	std::cout << tabs << "path: " << package.getPath() << "\n";
	std::cout << tabs << "projects:\n";
	for (auto const& project : package.getProjects()) {
		printProject(tabs, project);
	}
/*	if (not package.getPackages().empty()) {
		std::cout << tabs << "packages:\n";
		for (auto const& p : package.getPackages()) {
			for (auto const& project : package.getProjects()) {
				printProject(tabs, project);
			}

			std::cout << tabs << "  - name: " << p.getName() << "\n";
			printPackage(tabs + "    ", p);
		}
	}*/
}

auto groupPackages(std::vector<busy::analyse::Package const*> packages) -> std::map<busy::analyse::Package const*, std::vector<busy::analyse::Package const*>> {
	auto result = std::map<busy::analyse::Package const*, std::vector<busy::analyse::Package const*>>{};

	// finds a place in result
	// returns a pointer to an equivalent map key
	auto find = [&](busy::analyse::Package const* p) -> std::vector<busy::analyse::Package const*>* {
		for (auto& [id, list] : result) {
			if (p->isEquivalent(*id)) {
				return &list;
			}
		}
		return nullptr;
	};

	for (auto p : packages) {
		auto listPtr = find(p);
		if (listPtr) {
			listPtr->emplace_back(p);
		} else {
			result[p].push_back(p);
		}
	}
	return result;
}

auto collectDoublePackages(busy::analyse::Package const& package) -> std::map<std::string, std::vector<busy::analyse::Package const*>> {
	auto count = std::map<std::string, std::vector<busy::analyse::Package const*>>{};

	auto queue = std::queue<busy::analyse::Package const*>();
	queue.push(&package);
	while(not queue.empty()) {
		auto p = queue.front();
		queue.pop();
		count[p->getName()].push_back(p);
		for (auto const& p2 : p->getPackages()) {
			queue.push(&p2);
		}
	}
	return count;
}

auto groupPackagesByName(busy::analyse::Package const& package) {
	return collectDoublePackages(package);
}

auto getAllPackages(busy::analyse::Package const& package) {
	auto ret = std::map<std::string, busy::analyse::Package const*>{};

	auto queue = std::queue<busy::analyse::Package const*>();
	queue.push(&package);
	while(not queue.empty()) {
		auto e = queue.front();
		queue.pop();
		ret.try_emplace(e->getName(), e);
		for (auto const& p : e->getPackages()) {
			queue.push(&p);
		}
	}
	return ret;
}

auto findDependentProjects(busy::analyse::Package const& package, busy::analyse::Project const& project, std::map<std::string, busy::analyse::Package const*> packages) {
	auto ret = std::set<std::tuple<busy::analyse::Package const*, busy::analyse::Project const*>>{};

	auto _allIncludes = project.getIncludes();

	for (auto const& [key, exPackage] : packages) {
		for (auto const& exProject : exPackage->getProjects()) {
			for (auto file : exProject.getFiles().at(busy::analyse::FileType::H)) {
				auto path = exProject.getName() / relative(file.getPath(), exProject.getPath());
				if (_allIncludes.count(path)) {
					ret.emplace(nullptr, &exProject);
				}
			}
		}
	}
	return ret;
}

auto getAllProjects(std::map<std::string, busy::analyse::Package const*> packages) {
	auto ret = std::set<std::tuple<busy::analyse::Package const*, busy::analyse::Project const*>>{};
	for (auto const& [key, package] : packages) {
		for (auto const& project : package->getProjects()) {
			ret.emplace(std::tuple{package, &project});
		}
	}
	return ret;
}

namespace externalIssues {
	/// checks if <build>/external exists
	auto externalExists() {
		auto const external = std::filesystem::path{busy::analyse::Package::external};
		return exists(external);
	}

	/// collects all paths that should be removed from <build>/external
	auto invalidPaths() {
		auto invalidPaths = std::set<std::filesystem::path>{};

		auto const external = std::filesystem::path{busy::analyse::Package::external};
		if (not exists(external)) {
			return invalidPaths;
		}

		for (auto p : std::filesystem::directory_iterator{external}) {
			if (not p.is_symlink()) {
				invalidPaths.insert(p);
			} else if (not exists(relative(external / read_symlink(p)))) {
				invalidPaths.insert(p);
			}
		}
		return invalidPaths;
	}

	/// check for duplicate packages and potential version differences
	auto checkDuplicatePackages(busy::analyse::Package const& rootPackage) {
		auto listOfErrorGroups = std::vector<std::vector<std::vector<busy::analyse::Package const*>>>{};

		auto const external = std::filesystem::path{busy::analyse::Package::external};

		// check which other packages are available, if duplicates exists, check if they are the same
		// otherwise quit with an error
		auto groupedPackages = groupPackagesByName(rootPackage);

		groupedPackages.erase(rootPackage.getName()); //!TODO this should kind of be also an error?

		// all packages only available once, can be linked
		for (auto const& [name, packages] : groupedPackages) {
			// ignore if packages is linked at <build>/external or ./external
			if (exists(external / name) or exists(rootPackage.getPath() / external / name)) {
				continue;
			}

			// no issue if only one available package
			if (packages.size() == 1) {
				continue;
			}

			auto equalGroups = groupPackages(packages);
			// all version are the same, automatic choose the first
			if (equalGroups.size() == 1) {
				continue;
			}

			auto errorGroups = std::vector<std::vector<busy::analyse::Package const*>>{};
			for (auto const& [first, tail] : equalGroups) {
				errorGroups.emplace_back(tail);
			}
			listOfErrorGroups.emplace_back(std::move(errorGroups));
		}
		return listOfErrorGroups;
	}
}

auto fixExternalPath(busy::analyse::Package const& rootPackage) {
	auto externalExists = externalIssues::externalExists();
	auto invalidPaths   = externalIssues::invalidPaths();
	auto errorGroups    = externalIssues::checkDuplicatePackages(rootPackage);

	bool error = false;
	for (auto const& groups : errorGroups) {
		error = true;
		std::cout << "error with package " << groups.front().front()->getName() << "\n";
		for (auto const& group : groups) {
			std::cout << " -\n";
			for (auto const& p : group) {
				std::cout << "    - " << p->getPath() << "\n";
			}
		}
	}
	return error;
}

struct Project {
	busy::analyse::Package const* package;
	busy::analyse::Project const* project;

	std::set<Project const*> dependencies;
	std::set<Project const*> dependOnThis;
};

auto createProjects(std::map<std::string, busy::analyse::Package const*> const& allPackages) {
	std::vector<Project> projects;
	for (auto const& [package, project] : getAllProjects(allPackages)) {
		projects.push_back({package, project});
	}
	for (auto& p : projects) {
		std::cout << "checking " << p.project->getName() << "\n";
		auto deps = findDependentProjects(*p.package, *p.project, allPackages);

		for (auto& p2 : projects) {
			if (deps.count({nullptr, p2.project}) > 0) {
				if (&p != &p2) {
					p.dependencies.insert(&p2);
					p2.dependOnThis.insert(&p);
				}
			}
		}
	}

	return projects;
}

//!FixMe, should output a list, not do the output itself
bool checkForCycle(Project const& project) {
	std::queue<Project const*> projects;
	for (auto d : project.dependencies) {
		projects.push(d);
	}
	while (not projects.empty()) {
		auto top = projects.front();
		projects.pop();
		for (auto d : top->dependencies) {
			if (d == &project) {
				std::cerr << "dependency cycle between: " << project.package->getName() << "/" << project.project->getName() << " and " << top->package->getName() << "/" << top->project->getName() << "\n";
				return true;
			}
			projects.push(d);
		}
	}
	return false;
}
bool checkForCycle(std::map<std::string, busy::analyse::Package const*> packages) {
	auto projects = createProjects(packages);
	for (auto const& p : projects) {
		if (checkForCycle(p)) {
			return true;
		}
	}
	return false;
}

auto readFullFile(std::filesystem::path const& file) {
	auto ifs = std::ifstream{file, std::ios::binary};
	ifs.seekg(0, std::ios::end);
	auto buffer = std::vector<std::byte>(ifs.tellg());
	ifs.seekg(0, std::ios::beg);
	ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
	return buffer;
}

int main(int argc, char const** argv) {
	try {
		if (argc <= 1) {
			std::cout << "please give path of busy.yaml\n";
			return 0;
		}
		auto rootPath = relative(std::filesystem::path(argv[1]));
		if (rootPath == ".") {
			std::cout << "can't build in source, please create a `build` directory\n";
			return 0;
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

		{
			auto package = busy::analyse::Package{rootPath};

			// check consistency of packages
			std::cout << "check consistency\n";
			if (fixExternalPath(package)) {
				throw std::runtime_error("packages are not consistent");
			}

			auto packages = getAllPackages(package);
			if (checkForCycle(packages)) {
				throw std::runtime_error{"found packages that form a cycle"};
			}

			auto const& allPackages = packages;
			int compileCt{0};
			int linkCt{0};
			auto projects = createProjects(allPackages);
			for (auto const& p : projects) {
				auto countCpp = p.project->getFiles().at(busy::analyse::FileType::Cpp).size();
				auto countC   = p.project->getFiles().at(busy::analyse::FileType::C).size();
				auto cct      = countCpp + countC;
				if (cct > 0) {
					linkCt += 1;
				}
				compileCt += cct;
			}


			if (false) {
			for (auto const& p : projects) {
				std::cout << "  - project-name: " << p.package->getName() << "/" << p.project->getName() << "\n";
				std::cout << "    path: " << p.project->getPath() << "\n";
				if (not p.project->getSystemLibraries().empty()) {
					std::cout << "    systemLibraries:\n";
					for (auto const& l : p.project->getSystemLibraries()) {
						std::cout << "    - " << l << "\n";
					}
				}
				if (not p.dependencies.empty()) {
					std::cout << "    dependencies:\n";
					for (auto const& d : p.dependencies) {
						std::cout << "    - " << d->package->getName() << "/" << d->project->getName() << "\n";
					}
				}
				if (p.project->isHeaderOnly()) {
					std::cout << "    header-only: true\n";
				}
			}
			}

/*			printPackage("  ", package);
			for (auto const& p : package.getPackages()) {
				printPackage("  ", p);
			}*/
			std::cout << "link:" << linkCt << "\n";
			std::cout << "compile: " << compileCt << "\n";
			std::cout << "total: " << linkCt + compileCt << "\n";
			{
				using Q = Queue<busy::analyse::Project const, busy::analyse::File const>;
				auto nodes = Q::Nodes{};
				auto edges = Q::Edges{};

				auto projects = createProjects(allPackages);

				for (auto& p : projects) {
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

					auto fixExternal = [&](std::filesystem::path p) {
						if (isPrefix(std::filesystem::path{".."} / busy::analyse::Package::external, p)) {
							auto buildPath = relative(std::filesystem::current_path(), std::filesystem::current_path() / rootPath);
							return (buildPath / p).lexically_normal();
						}
						return p;
					};

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
							systemIncludes.push_back(x.getPath().parent_path());
							for (auto const& i : x.getLegacyIncludePaths()) {
								systemIncludes.push_back(i);
							}
						}
					});

					for (auto& p : projectIncludes) {
						p = fixExternal(p);
					}

					for (auto& p : systemIncludes) {
						p = fixExternal(p);
					}
					inFile  = fixExternal(inFile);
					outFile = fixExternal(outFile);

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
					params.emplace_back("obj" / relative(result->out, rootPath));
					params.emplace_back("-I");
					params.insert(end(params), begin(result->projectIncludes), end(result->projectIncludes));
					params.emplace_back("-isystem");
					params.insert(end(params), begin(result->systemIncludes), end(result->systemIncludes));
					return params;
				};
				auto linkLibrary = [&](busy::analyse::Project const& project) -> std::vector<std::string> {

					if (project.isHeaderOnly()) {
						return {};
					}

					auto target = (std::filesystem::path{"lib"} / project.getName()).replace_extension(".a");

					auto params = std::vector<std::string>{};
					params.emplace_back("./toolchainCall.sh");
					params.emplace_back("archive");
					params.emplace_back(canonical(rootPath));

					params.emplace_back(target);

					params.emplace_back("-i");
					for (auto file : queue.find_incoming<busy::analyse::File>(&project)) {
						auto ext = file->getPath().extension();
						if (ext == ".cpp" or ext == ".c") {
							auto objPath = "obj" / relative(file->getPath(), rootPath);
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
							auto objPath = "obj" / relative(file->getPath(), rootPath);
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
									for (auto const& p : projects) {
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
/*							auto p = std::string{};
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
							}
						}
					});
				}
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
	} catch (std::exception const& e) {
		std::cerr << "exception: " << exceptionToString(e) << "\n";
	}
}
