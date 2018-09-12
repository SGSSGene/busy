
#include "Package.h"

#include <queue>
#include <iostream>

void printProject(std::string tabs, busy::analyse::Project const& project) {
	std::cout << tabs << " - name: " << project.getName() << "\n";
	std::cout << tabs << "   path: " << project.getPath() << "\n";
	std::cout << tabs << "   files:\n";
	for (auto const& e : project.getFiles()) {
		if (e.second.empty()) continue;
		std::cout << tabs << "     - " << e.first << ":\n";
		for (auto const& f : e.second) {
			std::cout << tabs << "       - " << f.getPath() << "\n";
		}
	}
}

void printPackage(std::string tabs, busy::analyse::Package const& package) {
	std::cout << tabs << "path: " << package.getPath() << "\n";
	std::cout << tabs << "projects:\n";
	for (auto const& project : package.getProjects()) {
		printProject(tabs, project);
	}
	if (not package.getPackages().empty()) {
		std::cout << tabs << "packages:\n";
		for (auto const& p : package.getPackages()) {
			std::cout << tabs << "  - name: " << p.getName() << "\n";
			printPackage(tabs + "    ", p);
		}
	}
}

void findDoublePackages(busy::analyse::Package const& package) {
	struct Entry {
		int counter{0};
		std::vector<busy::analyse::Package const*> packages;
	};
	std::map<std::string, Entry> count;

	std::queue<busy::analyse::Package const*> queue;
	queue.emplace(&package);
	while(not queue.empty()) {
		auto const& front = *queue.front();
		queue.pop();
		auto& entry = count[front.getName()];
		entry.counter += 1;
		entry.packages.push_back(&front);
		for (auto const& p : front.getPackages()) {
			queue.emplace(&p);
		}
	}

	for (auto const& e : count) {
		//std::cout << "found " << e.first << " " << e.second.counter << " times\n";
		if (e.second.counter > 1) {
			auto const& first = *e.second.packages[0];
			bool error = false;
			for (int i{1}; i < e.second.packages.size(); ++i) {
				if (not first.isEquivalent(*e.second.packages[i])) {
					error = true;
				}
			}
			if (error) {
				std::cout << "Packages in different versions: " << first.getName() << " located at: " << "\n";
				for (auto p : e.second.packages) {
					std::cout << "  - " << p->getPath() << "\n";
				}
			}
		}
//		std::cout << e.first << " found " << e.second << " times\n";
	}
}

int main() {
	busy::analyse::Package package("./");

//	std::cout << "name: " << package.getName() << "\n";
	printPackage("", package);

//	std::cout << "---\n";
	findDoublePackages(package);
	// check for clashing packages
//	package.
}
