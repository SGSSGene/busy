#pragma once

#include "Package.h"
#include "Queue.h"

#include <map>

namespace busy::analyse {
using ProjectMap = std::map<Project const*, std::tuple<std::set<Project const*>, std::set<Project const*>>>;

struct CompilePipe {
	using Q        = Queue<busy::analyse::Project const, busy::analyse::File const>;
	using ColorMap = std::map<Q::Node, int>;

	std::string       toolchainCall;
	ProjectMap const& projects_with_deps;
	Q::Nodes          nodes;
	Q::Edges          edges;
	Q                 queue;
	ColorMap          colors;

	CompilePipe(std::string _toolchainCall, ProjectMap const& _projects_with_deps)
		: toolchainCall      {move(_toolchainCall)}
		, projects_with_deps {_projects_with_deps}
		, queue {loadQueue()}
	{}

	auto loadQueue() -> Q {
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
		return Queue{nodes, edges};
	}

	auto setupCompiling (busy::analyse::File const& file) -> std::vector<std::string> {
		auto outFile = file.getPath().lexically_normal().replace_extension(".o");
		auto inFile  = file.getPath();

		auto params = std::vector<std::string>{};

		params.emplace_back(toolchainCall);
		params.emplace_back("compile");
		params.emplace_back(inFile);
		params.emplace_back("obj" / outFile);

		// add all include paths
		params.emplace_back("-ilocal");

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
	}

	auto setupLinking(busy::analyse::Project const& project) {
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
	}

	bool empty() const {
		return queue.empty();
	}

	auto extract(busy::analyse::File const& file) -> std::vector<std::string> {
		return setupCompiling(file);
	};
	auto extract(busy::analyse::Project const& project) -> std::vector<std::string> {
		return setupLinking(project);
	};

	template <typename CB>
	void dispatch(CB cb) {
		queue.dispatch([&](auto& x) {
			auto params = extract(x);
			colors[&x] = cb(params);
		});
	}
};

}