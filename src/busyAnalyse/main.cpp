#include "FileCache.h"
#include "Package.h"

#include <process/Process.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <mutex>
#include <queue>

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

	count[package.getName()].push_back(&package);
	for (auto const& p : package.getPackages()) {
		count[p.getName()].push_back(&p);
		for (auto const& p2 : p.getPackages()) {
			count[p2.getName()].push_back(&p2);
		}
	}
	return count;
}

bool findDoublePackages(busy::analyse::Package const& package) {
	auto count = collectDoublePackages(package);

	bool error {false};
	for (auto const& [key, value] : count) {
		auto groups = groupPackages(value);
		if (groups.size() == 1) {
			continue;
		}
		auto rootPath = "./external/" + value.at(0)->getName() + "/";
		auto iter = std::find_if(begin(value), end(value), [&](auto const& e) {
			std::cout << "compare: " << rootPath << " " << e->getPath() << "\n";
			return rootPath == e->getPath();
		});

		if (iter == end(value)) {
			error = true;
		}
		std::cout << "Packages in different versions: " << groups.begin()->first->getName() << " located at: " << "\n";
		int type{1};
		for (auto const& [key, list] : groups) {
			std::cout << "  Type " << type << ":\n";
			for (auto p : list) {
				std::cout << "  - " << p->getPath() << "\n";
			}
			++type;
		}
	}
	return error;
}

bool linkPackages(busy::analyse::Package const& package) {
	std::queue<busy::analyse::Package const*> queue;
	for (auto const& p : package.getPackages()) {
		queue.emplace(&p);
	}
	bool addedNewLinks {false};
	while(not queue.empty()) {
		auto const& front = *queue.front();
		queue.pop();
		namespace fs = std::filesystem;
		// adding entries to package
		auto linkName = std::string{"./external/" + front.getName()};
		auto type = fs::status(linkName).type();
		if (type == fs::file_type::not_found) {
			addedNewLinks = true;
			auto targetPath = ".." / front.getPath();
			auto cmd = std::string{"ln -s " + targetPath.string() + " " + linkName};
			std::cout << cmd << "\n";
			system(cmd.c_str());
		} else if (type != fs::file_type::directory) {
			std::cout << "expected directory(or symbolic link) at: " << linkName << "\n";
		}

		for (auto const& p : front.getPackages()) {
			queue.emplace(&p);
		}
	}
	return addedNewLinks;
}

auto allIncludes(busy::analyse::Project const& project) {
	std::set<std::string> ret;
	for (auto const& [key, value] : project.getFiles()) {
		for (auto const& f : value) {
			for (auto const& i : f.getIncludes()) {
				ret.emplace(i);
			}
		}
	}
	return ret;
}
auto allOptionalIncludes(busy::analyse::Project const& project) {
	std::set<std::string> ret;
	for (auto const& [key, value] : project.getFiles()) {
		for (auto const& f : value) {
			for (auto const& i : f.getIncludesOptional()) {
				ret.emplace(i);
			}
		}
	}
	return ret;
}

auto allIncludables(busy::analyse::Project const& project) {
	std::set<std::string> ret;
	for (auto const& [key, value] : project.getFiles()) {
		for (auto const& f : value) {
			ret.emplace(f.getFlatPath());
		}
	}
	return ret;
}
auto getAllPackages(busy::analyse::Package const& package) {
	auto ret = std::map<std::string, busy::analyse::Package const*>{};

	ret[package.getName()] = &package;
	for (auto const& p : package.getPackages()) {
		ret[p.getName()] = &p;
	}
	return ret;
}

auto findDependentProjects(busy::analyse::Package const& package, busy::analyse::Project const& project, std::map<std::string, busy::analyse::Package const*> packages) {
	std::set<std::tuple<busy::analyse::Package const*, busy::analyse::Project const*>> ret;


	std::vector<busy::analyse::Package const*> p{&package};
	for (auto const& exPackage : package.getPackages()) {
		p.push_back(&exPackage);
	}
	auto _allIncludes = allIncludes(project);
	bool check = project.getName() == "testThreadPool";
	if (check) {
		std::cout << "find testThreadPool neededincludes:" << "\n";
		for (auto i : _allIncludes) {
			std::cout << "  - " << i << "\n";
		}
	}
	for (auto const& exPackage : p) {
		for (auto const& exProject : packages.at(exPackage->getName())->getProjects()) {
			auto includables = allIncludables(exProject);
			if (check) {
				std::cout << "checking if project has includes files: " << exProject.getName() << "\n";
				for (auto i : includables) {
					std::cout << "  - " << i << "\n";
				}
			}
			for (auto const& i : _allIncludes) {
				if (includables.count(i) > 0) {
					if (check) {
						std::cout << "found\n";
						std::cout << exProject.getPath()<< "\n";
					}
					ret.emplace(std::tuple{nullptr, &exProject});
				}
			}
		}
	}
	return ret;
}

void checkProject(busy::analyse::Package const& package, std::map<std::string, busy::analyse::Package const*> packages, busy::analyse::Project const& project) {
	std::cout << "\nchecking(2): " << project.getName() << "\n";

	std::vector<busy::analyse::Package const*> p{&package};
	for (auto const& exPackage : package.getPackages()) {
		p.push_back(&exPackage);
	}
	for (auto const& exPackage : p) {
		for (auto const& exProject : packages.at(exPackage->getName())->getProjects()) {
			auto includables = allIncludables(exProject);
			for (auto const& i : allIncludes(project)) {
				if (includables.count(i) > 0) {
					std::cout << "  should include " << exProject.getName() << "\n";
				}
			}
			for (auto const& i : allOptionalIncludes(project)) {
				if (includables.count(i) > 0) {
					std::cout << "  could include " << exProject.getName() << "\n";
				}
			}

		}
	}
}

void checkPackages(std::map<std::string, busy::analyse::Package const*> packages) {
	for (auto const& [key, value] : packages) {
		for (auto const& project : value->getProjects()) {
			checkProject(*value, packages, project);
		}
	}
}

auto getAllProjects(busy::analyse::Package const& rootPackage) {
	std::set<std::tuple<busy::analyse::Package const*, busy::analyse::Project const*>> ret;
	for (auto const& [key, package] : getAllPackages(rootPackage)) {
		for (auto const& project : package->getProjects()) {
			ret.emplace(std::tuple{package, &project});
		}
	}
	return ret;
}

struct Project {
	busy::analyse::Package const* package;
	busy::analyse::Project const* project;

	std::set<Project const*> dependencies;
	std::set<Project const*> dependOnThis;
};

auto createProjects(busy::analyse::Package const& package) {
	auto allPackages = getAllPackages(package);

	std::vector<Project> projects;
	for (auto const& [package, project] : getAllProjects(package)) {
		projects.push_back({package, project});
	}
	for (auto& p : projects) {
		auto deps = findDependentProjects(*p.package, *p.project, allPackages);

		bool check = p.project->getName() == "testThreadPool";
		if (check) {
			std::cout << "has dependencies: " << p.project->getName() << "\n";
			for (auto [key, value] : deps) {
				//std::cout << key->getPath() << " -> " << value->getPath() << "\n";
			}
		}

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
bool checkForCycle(busy::analyse::Package const& package) {
	auto projects = createProjects(package);
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

template <typename ...Args>
class Queue {
public:
	using Node = std::variant<Args*...>;
	using Edge = std::tuple<Node, Node>;
	using Nodes = std::vector<Node>;
	using Edges = std::vector<Edge>;

private:

	struct IntNode {
		Node value;
		int inNodeCt{0};
		std::vector<IntNode*> inNode;
		std::vector<IntNode*> outNode;
	};

	std::list<IntNode> nodes;

	std::queue<IntNode*> work;

	std::mutex mutex;

	auto& findNode(Node v) {
		for (auto& n : nodes) {
			if (n.value == v) {
				return n;
			}
		}
		assert(false);
	}
public:
	Queue(Nodes const& _nodes, Edges const& _edges) {
		for (auto n : _nodes) {
			nodes.emplace_back(IntNode{n, {}, {}});
		}
		for (auto [src, dst] : _edges) {
			auto& srcNode = findNode(src);
			auto& dstNode = findNode(dst);
			srcNode.outNode.push_back(&dstNode);
			dstNode.inNode.push_back(&srcNode);
		}

		for (auto& n : nodes) {
			if (n.inNode.size() == n.inNodeCt) {
				work.push(&n);
			}
		}
	}

	bool empty() const {
		return work.empty();
	}
	template <typename T>
	auto find_outgoing(Node n) -> T& {
		auto& node = findNode(n);
		for (auto const& n : node.outNode) {
			if (std::holds_alternative<T*>(n->value)) {
				return *std::get<T*>(n->value);
			}
		}
		assert(false);
	}
	template <typename L>
	void visit_incoming(Node n, L const& l) {
		auto& node = findNode(n);

		std::queue<IntNode*> work;

		for (auto n : node.inNode) {
			work.push(n);
		}

		while (not work.empty()) {
			auto front = work.front();
			work.pop();
			std::visit([&](auto& ptr) {
				l(*ptr);
			}, front->value);
			for (auto n : front->inNode) {
				work.push(n);
			}
		}
	}

	template <typename T>
	auto find_incoming(Node n) -> std::vector<T const*> {
		std::vector<T const*> result;
		auto& node = findNode(n);
		for (auto const& n : node.inNode) {
			if (std::holds_alternative<T const*>(n->value)) {
				result.push_back(std::get<T const*>(n->value));
			}
		}
		return result;
	}



	template <typename L>
	void dispatch(L const& l) {
		auto front = [&] {
			auto g = std::lock_guard{mutex};
			auto front = work.front();
			work.pop();
			return front;
		}();

		std::visit([&](auto ptr) {
			l(*ptr);
		}, front->value);

		{
			auto g = std::lock_guard{mutex};
			for (auto n : front->outNode) {
				n->inNodeCt += 1;
				if (n->inNode.size() == n->inNodeCt) {
					work.push(n);
				}
			}
		}
	}
};

int main(int argc, char const** argv) {
	try {
		auto const path = std::filesystem::path{[&]() {
			if (argc == 2) {
				return argv[1];
			} else {
				return "./";
			}
		}()};
		getFileCache() = [&]() {
			if (std::filesystem::exists(path / ".filecache")) {
				auto buffer = readFullFile(path / ".filecache");
				return fon::binary::deserialize<FileCache>(buffer);
			} else if (std::filesystem::exists(path / ".filecache.yaml")) {
				return fon::yaml::deserialize<FileCache>(YAML::LoadFile(path / ".filecache.yaml"));
			}
			return FileCache{};
		}();

		{
			auto package = busy::analyse::Package{path};
			std::cout << "---\n";
			if (findDoublePackages(package)) {
				throw std::runtime_error{"found duplicate packages with different versions"};
			}
			if (linkPackages(package)) {
				package = busy::analyse::Package{path};
			}
			if (checkForCycle(package)) {
				throw std::runtime_error{"found packages that form a cycle"};
			}

			int compileCt{0};
			int linkCt{0};
			auto projects = createProjects(package);
			for (auto const& p : projects) {
				auto countCpp = p.project->getFiles().at(busy::analyse::FileType::Cpp).size();
				auto countC   = p.project->getFiles().at(busy::analyse::FileType::C).size();
				auto cct      = countCpp + countC;
				if (cct > 0) {
					linkCt += 1;
				}
				compileCt += cct;
			}


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

				auto projects = createProjects(package);

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

				auto compileFile = [&](busy::analyse::File const& file) -> std::vector<std::string> {
					auto ext = file.getPath().extension();
					if (ext != ".cpp" and ext != ".c") {
						return {};
					}
					auto objPath = ".tmp/obj" / file.getPath();
					objPath.replace_extension(".o");


					auto params = std::vector<std::string>{};

					for (auto s : {"ccache", "g++", "-std=c++17", "-fPIC", "-MD", "-g3", "-ggdb", "-fdiagnostics-color=always"}) {
						params.emplace_back(s);
					}

					params.emplace_back("-c");
					params.emplace_back(file.getPath());
					params.emplace_back("-o");
					params.emplace_back(objPath);

					std::set<std::filesystem::path> inclSys;
					auto& project = queue.find_outgoing<busy::analyse::Project const>(&file);

					queue.visit_incoming(&project, [&](auto& x) {
						using X = std::decay_t<decltype(x)>;
						if constexpr (std::is_same_v<X, busy::analyse::Project>) {
							inclSys.insert(x.getPath().parent_path());
							for (auto const& i : x.getLegacyIncludePaths()) {
								inclSys.insert(i);
							}
						}
					});
					params.emplace_back("-I");
					params.emplace_back(project.getPath());
					params.emplace_back("-I");
					params.emplace_back(project.getPath().parent_path());
					params.emplace_back("-I");
					params.emplace_back(project.getPath().parent_path().parent_path() / "include");

					inclSys.erase(project.getPath().parent_path());
					for (auto const& p : inclSys) {
						params.emplace_back("-isystem");
						params.emplace_back(p);
					}

					std::filesystem::create_directories(objPath.parent_path());
					return params;
				};
				auto linkLibrary = [&](busy::analyse::Project const& project) -> std::vector<std::string> {

					if (project.isHeaderOnly()) {
						return {};
					}

					auto target = (std::filesystem::path{".tmp/lib"} / project.getName()).replace_extension(".a");

					auto params = std::vector<std::string>{};
					for (auto s : {"ccache", "ar", "rcs"}) {
						params.emplace_back(s);
					}
					params.emplace_back(target);

					for (auto file : queue.find_incoming<busy::analyse::File>(&project)) {
						auto ext = file->getPath().extension();
						if (ext == ".cpp" or ext == ".c") {
							auto objPath = ".tmp/obj" / file->getPath();
							objPath.replace_extension(".o");
							params.emplace_back(objPath);
						}
					}

					std::filesystem::create_directories(target.parent_path());

					return params;
				};
				auto linkExecutable = [&](busy::analyse::Project const& project) {
					auto target = std::filesystem::path{".tmp/bin"} / project.getName();

					auto params = std::vector<std::string>{};
					for (auto s : {"ccache", "g++", "-rdynamic", "-g3", "-ggdb", "-fdiagnostics-color=always"}) {
						params.emplace_back(s);
					}
					params.emplace_back("-o");
					params.emplace_back(target);


					for (auto file : queue.find_incoming<busy::analyse::File>(&project)) {
						auto ext = file->getPath().extension();
						if (ext == ".cpp" or ext == ".c") {
							auto objPath = ".tmp/obj" / file->getPath();
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
								auto target = (std::filesystem::path{".tmp/lib"} / project.getName()).replace_extension(".a");
								params.emplace_back(target);
							}
							addSystemLibraries(project);
						}
					});

					for (auto const& l : systemLibraries) {
						params.emplace_back("-l"+l);
					}

					std::filesystem::create_directories(target.parent_path());

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
							auto p = process::Process{params};
							if (p.getStatus() != 0) {
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
		// run compilation
		if (false)
		{
			busy::analyse::Package package("./");
			auto packages = getAllPackages(package);
			checkPackages(packages);
		}
		if (false)
		{
			busy::analyse::Package package("./");
			auto projects = createProjects(package);
			for (auto const& p : projects) {
				std::cout << "name: " << p.package->getName() << "/" << p.project->getName() << "\n";
				if (p.dependencies.size() > 0) {
					std::cout << "dep:\n";
					for (auto d : p.dependencies) {
						std::cout << "  - " << d->package->getName() << "/" << d->project->getName() << "\n";
					}
				}
				checkForCycle(p);

			}
		}

		{ // write binary
		auto node = fon::binary::serialize(getFileCache());
		auto ofs = std::ofstream{path / ".filecache", std::ios::binary};
		std::cout << "node: " << node.size() << "\n";
		ofs.write(reinterpret_cast<char const*>(node.data()), node.size());
		}
		if (false) { // write yaml
			YAML::Emitter out;
			out << fon::yaml::serialize(getFileCache());
			std::ofstream(path / ".filecache.yaml") << out.c_str();
		}
	} catch (std::exception const& e) {
		std::cerr << "exception: " << exceptionToString(e) << "\n";
	}
}
