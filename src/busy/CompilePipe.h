#pragma once

#include "Package.h"
#include "Queue.h"

#include <cstddef>
#include <map>

namespace busy {

// first tuple entry in going edges, second entry outgoing edges
using ProjectMap = std::map<Project const*, std::tuple<std::set<Project const*>, std::set<Project const*>>>;

struct CompilePipe {
	using Q        = Queue<busy::Project const, busy::File const>;
	enum class Color { Ignored, Compilable };
	using ColorMap = std::map<Q::Node, Color>;

	std::string       toolchainCall;
	ProjectMap const& projects_with_deps;
	std::set<std::string> const& toolchainOptions;
	Q::Nodes          nodes;
	Q::Edges          edges;
	Q                 queue;
	ColorMap          colors;
	std::mutex        mutex;

	CompilePipe(std::string _toolchainCall, ProjectMap const& _projects_with_deps, std::set<std::string> const& _toolchainOptions)
		: toolchainCall      {move(_toolchainCall)}
		, projects_with_deps {_projects_with_deps}
		, toolchainOptions   {_toolchainOptions}
		, queue {loadQueue()}
	{}

	[[nodiscard]]
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

	[[nodiscard]]
	auto setupCompiling (busy::File const& file) const -> std::vector<std::string> {
		auto outFile = file.getPath().lexically_normal().replace_extension(".o");
		auto inFile  = file.getPath();

		auto params = std::vector<std::string>{};

		params.emplace_back(toolchainCall);
		params.emplace_back("compile");
		params.emplace_back(inFile);
		params.emplace_back("obj" / outFile);
		// add all options
		params.emplace_back("-options");
		for (auto const& o : toolchainOptions) {
			params.emplace_back(o);
		}
		if (params.back() == "-options") params.pop_back();


		// add all include paths
		params.emplace_back("-ilocal");

		auto& project = queue.find_outgoing<busy::Project const>(&file);

		params.emplace_back(project.getPath());
		for (auto const& p : project.getLegacyIncludePaths()) {
			params.emplace_back(p);
		}
		if (params.back() == "-ilocal") params.pop_back();


		// add all system include paths
		params.emplace_back("-isystem");

		auto systemIncludes = std::vector<std::filesystem::path>{};
		queue.visit_incoming<Project const>(&project, [&](Project const& project) {
			params.emplace_back(project.getPath().parent_path());
			for (auto const& i : project.getLegacyIncludePaths()) {
				params.emplace_back(i);
			}
		});
		if (params.back() == "-isystem") params.pop_back();

		params.erase(std::unique(begin(params), end(params)), end(params));
		return params;
	}

	[[nodiscard]]
	auto setupLinking(busy::Project const& project) const {

		bool isExecutable = [&]() {
			if (project.getType() == "library") {
				return false;
			}
			return std::get<1>(projects_with_deps.at(&project)).empty();
		}();
		auto [action, target] = [&]() -> std::tuple<std::string, std::filesystem::path> {
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

		params.emplace_back("-options");
		for (auto const& o : toolchainOptions) {
			params.emplace_back(o);
		}
		if (params.back() == "-options") params.pop_back();


		params.emplace_back("-i");
		for (auto file : queue.find_incoming<busy::File>(&project)) {
			if (colors.at(file) == Color::Compilable) {
				auto objPath = "obj" / file->getPath();
				objPath.replace_extension(".o");
				params.emplace_back(objPath);
			}
		}
		if (params.back() == "-i") params.pop_back();


		params.emplace_back("-il");
		// add all legacy system libraries
		std::vector<std::string> systemLibraries;
		auto addSystemLibraries = [&](busy::Project const& project) {
			for (auto const& l : project.getSystemLibraries()) {
				auto iter = std::find(begin(systemLibraries), end(systemLibraries), l);
				if (iter != end(systemLibraries)) {
					systemLibraries.erase(iter);
				}
				systemLibraries.push_back(l);
			}
		};

		addSystemLibraries(project);

		auto dependencies = std::unordered_set<Project const*>{};
		queue.visit_incoming<Project const>(&project, [&](Project const& project) {
			if (colors.at(&project) == Color::Compilable) {
				auto target = (std::filesystem::path{"lib"} / project.getName()).replace_extension(".a");
				params.emplace_back(target);
			}
			addSystemLibraries(project);
			dependencies.insert(&project);
		});
		if (params.back() == "-il") params.pop_back();


		params.emplace_back("-l");
		for (auto const& l : systemLibraries) {
			params.emplace_back(l);
		}
		if (params.back() == "-l") params.pop_back();

		params.erase(std::unique(begin(params), end(params)), end(params));
		return std::tuple{params, dependencies};
	}

	[[nodiscard]]
	auto empty() const -> bool {
		return queue.empty();
	}

	[[nodiscard]]
	auto size() const -> size_t {
		return queue.size();
	}


	[[nodiscard]]
	auto extract(busy::File const& file) const -> std::tuple<std::vector<std::string>, std::nullptr_t> {
		return {setupCompiling(file), nullptr};
	}

	[[nodiscard]]
	auto extract(busy::Project const& project) const -> std::tuple<std::vector<std::string>, std::unordered_set<Project const*>> {
		return setupLinking(project);
	}

	auto pop() {
		return queue.pop();
	}
	template <typename Work, typename CB>
	void dispatch(Work work, CB const& cb) {
		queue.dispatch(work, [&](auto& x) {
			auto g = std::unique_lock{mutex};
			auto [params, dependencies] = extract(x);
			g.unlock();
			auto color = cb(x, params, dependencies);
			g.lock();
			colors[&x] = color;
		});
	}
};

}
