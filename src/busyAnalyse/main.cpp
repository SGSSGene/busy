#include "FileCache.h"
#include "Package.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
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
	for (auto const& exPackage : p) {
		for (auto const& exProject : packages.at(exPackage->getName())->getProjects()) {
			auto includables = allIncludables(exProject);
			for (auto const& i : allIncludes(project)) {
				if (includables.count(i) > 0) {
					ret.emplace(std::tuple{exPackage, &exProject});
				}
			}
		}
	}
	return ret;
}

void checkProject(busy::analyse::Package const& package, std::map<std::string, busy::analyse::Package const*> packages, busy::analyse::Project const& project) {
	std::cout << "\nchecking: " << project.getName() << "\n";

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

	std::vector<Project const*> dependent;
};

auto createProjects(busy::analyse::Package const& package) {
	auto allPackages = getAllPackages(package);

	std::vector<Project> projects;
	for (auto const& [package, project] : getAllProjects(package)) {
		projects.push_back({package, project});
	}
	for (auto& p : projects) {
		auto deps = findDependentProjects(*p.package, *p.project, allPackages);
		for (auto& p2 : projects) {
			if (deps.count({p2.package, p2.project}) > 0) {
				if (&p != &p2) {
					p.dependent.push_back(&p2);
				}
			}
		}
	}


	return projects;
}

bool checkForCycle(Project const& project) {
	std::queue<Project const*> projects;
	for (auto d : project.dependent) {
		projects.push(d);
	}
	while (not projects.empty()) {
		auto top = projects.front();
		projects.pop();
		for (auto d : top->dependent) {
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

			printPackage("  ", package);
			for (auto const& p : package.getPackages()) {
				printPackage("  ", p);
			}
			std::cout << "link:" << linkCt << "\n";
			std::cout << "compile: " << compileCt << "\n";
			std::cout << "total: " << linkCt + compileCt << "\n";


		}
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
				if (p.dependent.size() > 0) {
					std::cout << "dep:\n";
					for (auto d : p.dependent) {
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
