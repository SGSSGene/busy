#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <threadPool/threadPool.h>

namespace aBuild {


class NodeBase {
public:
	virtual ~NodeBase() {}
	virtual void const* getPtr() const = 0;
	bool operator==(NodeBase const& _other) {
		return getPtr() == _other.getPtr();
	}
	virtual bool call() const = 0;
};
template<typename T>
class Node : public NodeBase {
public:
	T* ptr;
	std::function<bool(T*)> func { [](T*){return true;} };
	~Node() override {}
	void const* getPtr() const override { return ptr; }
	bool call() const override { return func(ptr); }
};

class GraphNodeManager;

template<typename T>
class GraphNodeManagerBase {
public:
	static std::map<GraphNodeManager const*, std::map<T const*, Node<T>>> map;
};
template<typename T>
std::map<GraphNodeManager const*, std::map<T const*, Node<T>>> GraphNodeManagerBase<T>::map;



class GraphNodeManager {
private:
	std::map<void const*, std::function<void()>> clearFunctions;
public:
	~GraphNodeManager() {
		for (auto e : clearFunctions) {
			e.second();
		}
	}

	template<typename T>
	Node<T> const* addNode(T* _ptr) {
		GraphNodeManagerBase<T>::map[this][_ptr].ptr = _ptr;
		clearFunctions[&GraphNodeManagerBase<T>::map] = [this] { GraphNodeManagerBase<T>::map[this].clear(); };
		return &GraphNodeManagerBase<T>::map[this][_ptr];
	}

	template<typename T>
	Node<T> const* addNode(T* _ptr, std::function<bool(T*)> _func) {
		GraphNodeManagerBase<T>::map[this][_ptr].ptr  = _ptr;
		GraphNodeManagerBase<T>::map[this][_ptr].func = _func;

		clearFunctions[&GraphNodeManagerBase<T>::map] = [this] { GraphNodeManagerBase<T>::map[this].clear(); };
		return &GraphNodeManagerBase<T>::map[this][_ptr];
	}

	template<typename T>
	Node<T> const* getNode(T const* _ptr) const {
		return &GraphNodeManagerBase<T>::map.at(this).at(_ptr);
	}
	template <typename T>
	std::vector<Node<T> const*> getNodes(std::vector<T const*> const& _ptrs) const {
		std::vector<Node<T> const*> retList;
		retList.reserve(_ptrs.size());
		for (auto p : _ptrs) {
			retList.push_back(getNode(p));
		}
		return retList;
	}

};

class Graph {
private:
	std::set<NodeBase const*> mNodes;
	std::set<std::pair<NodeBase const*, NodeBase const*>> mEdges;

	GraphNodeManager nodeManager;
public:
	template<typename T>
	void addNode(T* _node, std::function<void(T*)> _func) {
		addNode(_node, [=](T* _t) {
			_func(_t);
			return true;
		});
	}

	template<typename T>
	void addNode(T* _node, std::function<bool(T*)> _func) {
		auto p = nodeManager.addNode(_node, _func);
		mNodes.insert(p);
	}


	template<typename T1, typename T2>
	void addEdge(T1* _src, T2* _to) {
		auto p1 = nodeManager.addNode(_src);
		auto p2 = nodeManager.addNode(_to);

		mNodes.insert(p1);
		mNodes.insert(p2);

		mEdges.insert({p1, p2});
	}

	auto getIngoing(NodeBase const* _node) const -> std::set<NodeBase const*> {
		std::set<NodeBase const*> retSet;
		for (auto const& e : mEdges) {
			if (e.second == _node) {
				retSet.insert(e.first);
			}
		}
		return retSet;
	}
	auto getOutgoing(NodeBase const* _node) const -> std::set<NodeBase const*> {
		std::set<NodeBase const*> retSet;
		for (auto const& e : mEdges) {
			if (e.first == _node) {
				retSet.insert(e.second);
			}
		}
		return retSet;
	}



	template<typename T1, typename T2>
	auto getIngoing(T2* _node, bool recursive) const -> std::set<T1*> {
		auto node = nodeManager.getNode(_node);
		return getIngoingImpl<T1>(node, recursive);
	}

	template<typename T1>
	auto getIngoingImpl(NodeBase const* _node, bool recursive) const -> std::set<T1*> {
		std::set<T1*> retSet;

		for (auto const& e : mEdges) {
			if (e.second == _node) {
				auto ptr = dynamic_cast<Node<T1> const*>(e.first);
				if (ptr != nullptr) {
					retSet.insert(ptr->ptr);
				}
				if (recursive) {
					for (auto const& r : getIngoingImpl<T1>(e.first, recursive)) {
						retSet.insert(r);
					}
				}
			}
		}
		return retSet;
	}



	template<typename T1, typename T2>
	auto getOutgoing(T2* _node, bool recursive) const -> std::set<T1*> {
		auto node = nodeManager.getNode(_node);
		return getOutgoingImpl<T1>(node, recursive);
	}

	template<typename T1>
	auto getOutgoingImpl(NodeBase const* _node, bool recursive) const -> std::set<T1*> {
		std::set<T1*> retSet;

		for (auto const& e : mEdges) {
			if (e.first == _node) {
				auto ptr = dynamic_cast<Node<T1> const*>(e.second);
				if (ptr != nullptr) {
					retSet.insert(ptr->ptr);
				}
				if (recursive) {
					for (auto const& r : getOutgoingImpl<T1>(e.second, recursive)) {
						retSet.insert(r);
					}
				}
			}
		}
		return retSet;
	}

	template<typename T1>
	void removeUnreachableIngoing(std::vector<T1 const*> const& _nodes) {
		auto nodes = nodeManager.getNodes(_nodes);
		auto allNodes = mNodes;
		std::set<NodeBase const*> reachableNodes;
		getReachableIngoingImpl(nodes, reachableNodes);

		for (auto n : reachableNodes) {
			allNodes.erase(n);
		}
		for (auto& n : nodes) {
			allNodes.erase(n);
		}

		for (auto iter = mEdges.begin(); iter != mEdges.end();) {
			if (allNodes.count(iter->first) > 0 || allNodes.count(iter->second) > 0) {
				iter = mEdges.erase(iter);
			} else {
				++iter;
			}
		}
		mNodes = reachableNodes;
		for (auto& n : nodes) {
			mNodes.insert(n);
		}
	}
	template <typename T>
	void getReachableIngoingImpl(std::vector<Node<T> const*> const& _nodes, std::set<NodeBase const*>& _retVal) {
		for (auto n : _nodes) {
			getReachableIngoingImpl(n, _retVal);
		}
	}

	void getReachableIngoingImpl(NodeBase const* _node, std::set<NodeBase const*>& _retVal) {
		for (auto const& e : mEdges) {
			if (e.second == _node) {
				_retVal.insert(e.first);
				getReachableIngoingImpl(e.first, _retVal);
			}
		}
	}


	bool visitAllNodes(int threadCt = 1, std::function<void(int, int, int)> _monitor = [](int, int, int){}) {
		std::atomic_bool success { true };
		std::map<NodeBase const*, std::set<NodeBase const*>> ingoingNodes;
		for (auto& n : mNodes) {
			ingoingNodes[n] = getIngoing(n);
		}
		int totaltotal = mNodes.size();
		int total = 0;
		int done = 0;

		_monitor(done, total, totaltotal);

		std::mutex mutex;
		threadPool::ThreadPool<NodeBase const*> threadPool;
		threadPool.spawnThread([&](NodeBase const* top) {
			// Only run, if no error occured
			if (success) {
				bool _success = top->call();
				if (not _success) {
					success = false;
				}
			}
			{
				std::unique_lock<std::mutex> lock(mutex);
				auto outgoing = getOutgoing(top);
				for (auto const& o : outgoing) {
					for (auto iter = ingoingNodes.begin(); iter != ingoingNodes.end();) {
						if (iter->first == o) {
							iter->second.erase(top);
							if (iter->second.empty()) {
								++total;
								threadPool.queue(iter->first);
//								openList.push_back(iter->first);
								iter = ingoingNodes.erase(iter);
							} else {
								++iter;
							}
						} else {
							++iter;
						}
					}
				}
				++done;
				_monitor(done, total, totaltotal);

			}

		}, threadCt);

		// queue all nodeBases without ingoing edges
		std::vector<NodeBase const*> openList;
		{
			std::unique_lock<std::mutex> lock(mutex);
			for (auto iter = ingoingNodes.begin(); iter != ingoingNodes.end();) {
				if (iter->second.empty()) {
					++total;
					threadPool.queue(iter->first);
					//openList.push_back(iter->first);
					iter = ingoingNodes.erase(iter);
				} else {
					++iter;
				}
			}
		}
		threadPool.wait();
		return success;
	}
};

}
