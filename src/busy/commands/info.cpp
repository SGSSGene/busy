#include "commands.h"

#include "NeoVisitor.h"
#include "NeoWorkspace.h"

#include <iostream>

using namespace busy;

namespace commands {

auto sanitize(std::string name) -> std::string {
	for (auto& c : name) {
		if ((c >= 'A' and c <= 'Z')
		    or (c >= 'a' and c <= 'z')
		    or (c >= '0' and c <= '9')) {
		    c = std::toupper(c);
		} else {
			c = '_';
		}
	}
	return name;
}



namespace {
class TablePrint {
	std::vector<std::vector<std::string>> mEntries;
	std::vector<int> mColumnSize;

public:
	void add(std::vector<std::string> line) {
		for (int i(0); i < int(line.size()); ++i) {
			auto const& e = line.at(i);
			if (i >= int(mColumnSize.size())) {
				mColumnSize.emplace_back(0);
			}
			if (mColumnSize.at(i) < int(e.size())) {
				mColumnSize[i] = int(e.size());
			}
		}
		mEntries.emplace_back(std::move(line));
	}
	void print() {
		for (auto const& line : mEntries) {
			for (int i(0); i < int(line.size()); ++i) {
				auto const& e = line.at(i);
				printFilled(e, mColumnSize.at(i) + 1);
			}
			std::cout << std::endl;
		}
	}
private:
	void printFilled(std::string const& _str, int n) {
		int fillUp = n - int(_str.size());
		std::cout << _str;
		for (int i(0); i < fillUp; ++i) {
			std::cout << " ";
		}
	}
};

template <typename T>
void printList(std::string const& _title, std::vector<T> const& _list, std::function<void(T const&)> _func = [](T const& t) { std::cout << t; }) {
	if (_list.empty()) return;
	std::cout << _title << std::endl;
	for (auto const& s : _list) {
		std::cout << "  - ";
		_func(s);
		std::cout << std::endl;
	}
}
}

void info(std::vector<std::string> str) {

	NeoWorkspace ws;

	if (str.at(0) == "packageFolders" && str.size() == 1) {
		std::cout << "PackageFolders: " << std::endl;
		for (auto const& f : ws.getPackageFolders()) {
			std::cout << "  - " << f << std::endl;
		}
	} else if (str.at(0) == "packages" && str.size() == 1) {
		std::cout << "Packages:" << std::endl;
		for (auto const& package : ws.getPackages()) {
			std::cout << "  - " << package.getName() << std::endl;
		}
	} else if (str.at(0)  == "projects" && str.size() == 1) {
		TablePrint tp;
		tp.add({"project", "", "path", "has config entry"});
		for (auto package : ws.getPackages()) {
			for (auto project : package.getProjects()) {
				tp.add({project.getFullName(), "found at", project.getPath(), project.getHasConfigEntry() ? "true" : "false"});
			}
		}
		tp.print();
	} else if (str.at(0) == "package" && str.size() == 2) {
		auto package = ws.getPackage(str.at(1));

		std::cout << "Package: " << package.getName() << std::endl;
		std::cout << "  - Projects: " << std::endl;
		for (auto const& project : package.getProjects()) {
			std::cout << "    * " << package.getName() << "/" << project.getName() << std::endl;
		}
		std::cout << "  - External Repositories: " << std::endl;
		for (auto const& rep : package.getExternalPackageURLs()) {
			std::cout << "    * " << rep.name << " " << rep.url << " " << rep.branch << std::endl;
		}
	} else if (str.at(0) == "project" && str.size() == 2) {
		auto const& project = ws.getProject(str.at(1));
		std::cout << "Project: " << project.getFullName() << std::endl;

		std::cout << "Type: " << project.getType() << std::endl;
		std::cout << "has entry in busy.yaml: " << std::boolalpha << project.getHasConfigEntry() << std::endl;
		std::cout << "link whole archive: " << project.getWholeArchive() << std::endl;
		std::cout << "auto dependency discovery: " << project.getAutoDependenciesDiscovery() << std::endl;
		printList<NeoProject const*>("Dependencies:",  project.getDependencies(), [] (NeoProject const* const& p) { std::cout << p->getFullName(); });
		printList("Source Paths:",  project.getSourcePaths());
		printList("Include Paths:", project.getIncludePaths());
		printList("System Include Paths:", project.getSystemIncludePaths());
/*		printList("C-Files:",       project.getCFiles());
		printList("C++-Files:",     project.getCppFiles());
		printList("Include-Files:", project.getIncludeFiles());*/
//		printList("Defines:",       project.getDefines());
	} else if (str.at(0) == "compile") {
		std::cout << "available toolchains: " << std::endl;
		for (auto const& toolchain : ws.getToolchains()) {
			std::cout << "  - " << toolchain.first << std::endl;
		}
		std::cout << "picking system-gccc as toolchain" << std::endl;
		auto toolchain = ws.getToolchains().at("system-gcc");

		std::cout << "available flavors: " << std::endl;
		for (auto const& flavor : ws.getFlavors()) {
			std::cout << "  - " << flavor.first << std::endl;
		}
		std::cout << "picking busy/default flavor" << std::endl;

		auto flavor = ws.getFlavors().at("busy/default");

		NeoVisitor visitor(ws, str.size() > 1 ? str.at(1):"");
		visitor.setProjectVisitor([] (NeoProject const* _project) {
			std::vector<std::string> options;
			options.push_back("-std=c++11");
			options.push_back("-DBUSY");
			options.push_back("-DBUSY_" + sanitize(_project->getName()));
			for (auto depP : _project->getDependencies()) {
				options.push_back("-DBUSY_" + sanitize(depP->getName()));
			}
			for (auto path : _project->getIncludeAndDependendPaths()) {
				options.push_back("-I " + path);
			}
			for (auto path : _project->getSystemIncludeAndDependendPaths()) {
				options.push_back("-isystem " + path);
			}
			for (auto const& o : options) {
				std::cout << o << " ";
			}
			std::cout << std::endl;
		});
		visitor.visit(1);
	}
}
}
