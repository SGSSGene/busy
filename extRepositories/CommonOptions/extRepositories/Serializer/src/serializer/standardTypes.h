#pragma once

#ifdef BUSY_GENERICFACTORY
#include <genericFactory/genericFactory.h>
#endif

#include "Converter.h"

#include <array>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>


namespace serializer {
	template<typename T>
	class Converter<std::vector<T>> {
	public:
		template<typename Adapter>
		static void serialize(Adapter& adapter, std::vector<T>& x) {
			adapter.serializeByIter(x.begin(), x.end());
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, std::vector<T>& x) {
			x.clear();
			std::function<void(T&)> func = [&x](T& v) {
				x.emplace_back(std::move(v));
			};
			adapter.deserializeByInsert(func);
		}
	};

	template<typename T, size_t N>
	class Converter<std::array<T, N>> {
	public:
		template<typename Adapter>
		static void serialize(Adapter& adapter, std::array<T, N>& x) {
			adapter.serializeByIter(x.begin(), x.end());
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, std::array<T, N>& x) {
			size_t idx = 0;
			std::function<void(T&)> func = [&x, &idx](T& v) {
				x[idx++] = std::move(v);
			};
			adapter.deserializeByInsert(func);
		}
	};


	template<typename T>
	class Converter<T, typename std::enable_if<std::is_enum<T>::value>::type> {
	private:
		using Type = typename std::underlying_type<T>::type;
	public:
		template<typename Adapter>
		static void serialize(Adapter& adapter, T& x) {
			Type value = Type(x);
			adapter.serialize(value);
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, T& x) {
			Type value;
			adapter.deserialize(value);
			x = T(value);
		}
	};


	template<typename T>
	class Converter<std::list<T>> {
	public:
		template<typename Adapter>
		static void serialize(Adapter& adapter, std::list<T>& x) {
			adapter.serializeByIter(x.begin(), x.end());
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, std::list<T>& x) {
			x.clear();
			std::function<void(T&)> func = [&x](T& v) {
				x.push_back(v);
			};
			adapter.deserializeByInsert(func);
		}
	};

	template<typename T>
	class Converter<std::set<T>> {
	public:
		template<typename Adapter>
		static void serialize(Adapter& adapter, std::set<T>& x) {
			adapter.serializeByIterCopy(x.begin(), x.end());
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, std::set<T>& x) {
			x.clear();
			std::function<void(T&)> func = [&x](T& v) {
				x.insert(v);
			};
			adapter.deserializeByInsert(func);
		}
	};

	template<typename Key, typename Value>
	class Converter<std::pair<Key, Value>> {
	public:
		template<typename PKey, typename PValue>
		struct Pair {
			std::pair<PKey, PValue>& pair;
			template<typename Node>
			void serialize(Node& node) {
				node["first"]  % pair.first;
				node["second"] % pair.second;
			}
		};
		template<typename Adapter>
		static void serialize(Adapter& adapter, std::pair<Key, Value>& x) {
			Pair<Key, Value> pair {x};
			adapter.serialize(pair);
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, std::pair<Key, Value>& x) {
			Pair<Key, Value> pair {x};
			adapter.deserialize(pair);
		}
	};

	template<typename Key, typename Value>
	class Converter<std::map<Key, Value>> {
	public:
		template<typename Adapter>
		static void serialize(Adapter& adapter, std::map<Key, Value>& x) {
			adapter.serializeMapByIter(x.begin(), x.end());
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, std::map<Key, Value>& x) {
			x.clear();
			adapter.deserializeMap(x);
		}
	};

	template<>
	class Converter<std::string> {
	public:
		template<typename Adapter>
		static void serialize(Adapter& adapter, std::string& x) {
			std::vector<uint8_t> v(x.size()+1, '\0');
			memcpy(&v[0], x.c_str(), x.size());
			adapter.serialize(v);
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, std::string& x) {
			std::vector<uint8_t> v;
			adapter.deserialize(v);
			x = (char*)v.data();
		}
	};


	template<typename T>
	class Converter<std::unique_ptr<T>> {
	public:
		struct SerUPtr {
			SerUPtr() : ptr {nullptr} {}
			SerUPtr(std::unique_ptr<T>* _ptr)
				: ptr (_ptr) {}

			std::unique_ptr<T>* ptr;
#ifndef BUSY_GENERICFACTORY
			template<typename Node>
			void serialize(Node& node) {
				bool valid = ptr != nullptr;
				node["valid"] % valid;
				if (valid) {
					node["data"] % *(ptr->get());
				}
			}
#else
			template<typename Node>
			void serialize(Node& node) {
				bool valid = ptr != nullptr;
				node["valid"] % valid;
				if (valid && genericFactory::hasType(ptr->get())) {
					auto type = genericFactory::GenericFactory::getInstance().getType(ptr->get());
					node["type"] % type;
					node["data"] % *(ptr->get());
				} else {
					node["data"] % *(ptr->get());
				}
			}

#endif
		};
		struct DeserUPtr {
			DeserUPtr() : ptr {nullptr} {}
			DeserUPtr(std::unique_ptr<T>* _ptr)
				: ptr (_ptr) {}
			std::unique_ptr<T>* ptr;

#ifndef BUSY_GENERICFACTORY
			template<typename Node>
			void serialize(Node& node) {
				bool valid;
				node["valid"] % valid;
				if (valid) {
					ptr->reset(new T{});
					node["data"] % *(ptr->get());
				} else {
					ptr->reset();
				}
			}
#else
			template<typename T2, typename std::enable_if<std::is_default_constructible<T2>::value>::type* = nullptr>
			T2* getDefault() {
				return new T2();
			}
			template<typename T2, typename std::enable_if<not std::is_default_constructible<T2>::value>::type* = nullptr>
			T2* getDefault() {
				return nullptr;
			}

			template<typename Node>
			void serialize(Node& node) {
				bool valid;
				node["valid"] % valid;
				if (valid) {
					std::string type;
					node["type"] % type;
					if (type != "" and genericFactory::getClassList<T>().count(type) > 0) {
						*ptr = genericFactory::make_unique<T>(type);
					} else {
						ptr->reset(getDefault<T>());
					}
					if (*ptr) {
						node["data"] % *(ptr->get());
					}
				} else {
					ptr->reset();
				}
			}
#endif
		};

		template<typename Adapter>
		static void serialize(Adapter& adapter, std::unique_ptr<T>& x) {
			SerUPtr ptr (&x);
			adapter.serialize(ptr);
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, std::unique_ptr<T>& x) {
			DeserUPtr ptr (&x);
			adapter.deserialize(ptr);
		}
	};

}


