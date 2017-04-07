#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <typeinfo>
#include <type_traits>
#include <vector>


#ifdef BUSY_SERIALIZER
#include <serializer/has_serialize_function.h>

	namespace serializer {
	namespace binary {
		class SerializerNode;
		class DeserializerNode;
	}
	namespace json {
		class SerializerNode;
		class DeserializerNode;
	}
	namespace yaml {
		class SerializerNode;
		class DeserializerNode;
	}}
#endif


namespace genericFactory {

class Base {
public:
	virtual ~Base() {}
};

template<typename B>
class BaseT : public Base {
private:
	virtual std::shared_ptr<B> createSharedBase()     const = 0;
	virtual B*                 createUniqueBase()     const = 0;
	virtual void               copyBase(B*, B const*) const = 0;
public:
#ifdef BUSY_SERIALIZER
	virtual void serialize(B* _base, serializer::binary::SerializerNode& node)   const = 0;
	virtual void serialize(B* _base, serializer::binary::DeserializerNode& node) const = 0;
	virtual void serialize(B* _base, serializer::json::SerializerNode& node)   const = 0;
	virtual void serialize(B* _base, serializer::json::DeserializerNode& node) const = 0;
	virtual void serialize(B* _base, serializer::yaml::SerializerNode& node)   const = 0;
	virtual void serialize(B* _base, serializer::yaml::DeserializerNode& node) const = 0;
#endif
	template<typename T2>
	std::shared_ptr<T2> createShared() const {
		auto ptr1 = createSharedBase();
		auto ptr2 = std::dynamic_pointer_cast<T2>(ptr1);
		if (ptr2.get() == nullptr) {
			throw std::runtime_error("can't create shared pointer (genericFactory)");
		}
		return ptr2;
	}
	template<typename T2>
	std::unique_ptr<T2> createUnique() const {
		auto ptr1 = createUniqueBase();
		auto ptr2 = dynamic_cast<T2*>(ptr1);
		if (ptr2 == nullptr) {
			delete ptr1;
			throw std::runtime_error("can't create unique pointer (genericFactory)");
		}
		return std::unique_ptr<T2> { ptr2 };
	}
	template<typename T2>
	void copy(T2* ptr1, T2 const* ptr2) const {
		copyBase(ptr1, ptr2);
	}
};


template<typename B, typename T>
class BaseTT final : public BaseT<B> {
public:
	std::shared_ptr<B> createSharedBase() const override {
		return std::make_shared<T>();
	}
	B* createUniqueBase() const override {
		return new T();
	}
	void copyBase(B* _ptr1, B const* _ptr2) const override {
		copyBaseImpl<T>(_ptr1, _ptr2);
	}

private:
	template <typename Type, typename std::enable_if<not std::is_assignable<Type, Type>::value>::type* = nullptr>
	void copyBaseImpl(B* _ptr1, B const* _ptr2) const {
		throw std::runtime_error("copying class, that is not assignable");
	}

	template <typename Type, typename std::enable_if<std::is_assignable<Type, Type>::value>::type* = nullptr>
	void copyBaseImpl(B* _ptr1, B const* _ptr2) const {
		auto ptr1 = dynamic_cast<T*>(_ptr1);
		auto ptr2 = dynamic_cast<T const*>(_ptr2);
		*ptr1 = *ptr2; //!WARNING If compilation fails in this line, explicitly delete your assignment operator
	}
public:
#ifdef BUSY_SERIALIZER
	void serialize(B* _base, serializer::binary::SerializerNode& node) const override{
		implSerialize(dynamic_cast<T*>(_base), node);
	}
	void serialize(B* _base, serializer::binary::DeserializerNode& node) const override{
		implSerialize(dynamic_cast<T*>(_base), node);
	};
	void serialize(B* _base, serializer::json::SerializerNode& node) const override {
		implSerialize(dynamic_cast<T*>(_base), node);
	}
	void serialize(B* _base, serializer::json::DeserializerNode& node) const override {
		implSerialize(dynamic_cast<T*>(_base), node);
	};
	void serialize(B* _base, serializer::yaml::SerializerNode& node) const override {
		implSerialize(dynamic_cast<T*>(_base), node);
	}
	void serialize(B* _base, serializer::yaml::DeserializerNode& node) const override {
		implSerialize(dynamic_cast<T*>(_base), node);
	};

private:
	template<typename Node, typename Type, typename std::enable_if<serializer::has_serialize_function<Type, Node>::value>::type* = nullptr>
	void implSerialize(Type* _type, Node& node) const {
		_type->serialize(node);
	}
	template<typename Node, typename Type, typename std::enable_if<not serializer::has_serialize_function<Type, Node>::value>::type* = nullptr>
	void implSerialize(Type* _type, Node& node) const {
		throw std::runtime_error("can't serialize this genericFactory item, because it has no serialize function");
	}
#endif
};

class GenericFactory {
private:
	std::map<std::size_t, std::string>                        classList;
	std::map<std::string, std::vector<std::unique_ptr<Base>>> constructorList;
	std::map<std::string, std::vector<std::unique_ptr<Base>>> copyList;
	std::map<std::size_t, std::set<std::string>>              inheritanceMap;

	mutable std::recursive_mutex mMutex;

	struct RecorderItem {
		std::size_t mBaseType;
		std::vector<std::string> mNames;
	};
	std::map<std::thread::id, RecorderItem> mItemRecorder;

public:
	static GenericFactory& getInstance() {
		static GenericFactory instance;
		return instance;
	}

	template <typename T>
	void activateRegisterRecorder() {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		mItemRecorder[std::this_thread::get_id()] = { typeid(T).hash_code(), {}};
	}
	auto deactivateRegisterRecorder() -> std::vector<std::string> {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		auto retValue = std::move(mItemRecorder[std::this_thread::get_id()].mNames);
		mItemRecorder.erase(std::this_thread::get_id());
		return retValue;
	}
	template <typename T>
	void registerRecorderItem(std::string const& _name) {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		auto iter = mItemRecorder.find(std::this_thread::get_id());
		if (iter != mItemRecorder.end() and typeid(T).hash_code() == iter->second.mBaseType) {
			iter->second.mNames.push_back(_name);
		}
	}

	template<typename T>
	std::set<std::string> getValidNames() const {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		auto hash = typeid(T).hash_code();
		if (inheritanceMap.count(hash) == 0) {
			return {};
		}
		return inheritanceMap.at(hash);
	}

	template<typename T, typename std::enable_if<std::is_abstract<T>::value>::type* = nullptr>
	void registerClass(std::string const& _name) {
		static_assert(std::is_polymorphic<T>::value, "must be polymorphic type");
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		classList[typeid(T).hash_code()] = _name;
	}

	template<typename T, typename std::enable_if<not std::is_abstract<T>::value>::type* = nullptr>
	void registerClass(std::string const& _name) {
		static_assert(std::is_polymorphic<T>::value, "must be polymorphic type");
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		classList[typeid(T).hash_code()] = _name;
		constructorList[_name].emplace_back(new BaseTT<T, T>());
		inheritanceMap[typeid(T).hash_code()].insert(_name);

		registerRecorderItem<T>(_name);
	}

	template<typename T, typename BASE, typename ...Bases>
	void registerClass(std::string const& _name) {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		registerClass<T, Bases...>(_name);
		classList[typeid(T).hash_code()] = _name;
		constructorList[_name].emplace_back(new BaseTT<BASE, T>());
		inheritanceMap[typeid(BASE).hash_code()].insert(_name);
		registerRecorderItem<BASE>(_name);
	}

	template<typename T, typename std::enable_if<std::is_abstract<T>::value>::type* = nullptr>
	void unregisterClass(std::string const& _name) {
		static_assert(std::is_polymorphic<T>::value, "must be polymorphic type");
		std::lock_guard<std::recursive_mutex> lock(mMutex);

		auto iter = classList.find(typeid(T).hash_code());
		if (iter != classList.end()) {
			classList.erase(iter);
		}
	}

	template<typename T, typename std::enable_if<not std::is_abstract<T>::value>::type* = nullptr>
	void unregisterClass(std::string const& _name) {
		static_assert(std::is_polymorphic<T>::value, "must be polymorphic type");
		std::lock_guard<std::recursive_mutex> lock(mMutex);

		{
			auto iter = classList.find(typeid(T).hash_code());
			if (iter != classList.end()) {
				classList.erase(iter);
			}
		}
		{
			auto iter = constructorList.find(_name);
			if (iter != constructorList.end()) {
				constructorList.erase(iter);
			}
		}
		{
			auto iter = inheritanceMap.at(typeid(T).hash_code()).find(_name);
			if (iter != inheritanceMap.at(typeid(T).hash_code()).end()) {
				inheritanceMap[typeid(T).hash_code()].erase(iter);
			}
		}
	}

	template<typename T, typename BASE, typename ...Bases>
	void unregisterClass(std::string const& _name) {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		registerClass<T, Bases...>(_name);

		{
			auto iter = classList.find(typeid(T).hash_code());
			if (iter != classList.end()) {
				classList.erase(iter);
			}
		}
		{
			auto iter = constructorList.find(_name);
			if (iter != constructorList.end()) {
				constructorList.erase(iter);
			}
		}
		{
			auto iter = inheritanceMap.at(typeid(BASE).hash_code()).find(_name);
			if (iter != inheritanceMap.at(typeid(BASE).hash_code()).end()) {
				inheritanceMap[typeid(BASE).hash_code()].erase(iter);
			}
		}
	}


	template<typename T>
	std::string const& getType() const {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		return classList.at(typeid(T).hash_code());
	}

	template<typename T>
	std::string const& getType(T* t) {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		return classList.at(typeid(*t).hash_code());
	}

	template<typename T>
	bool hasType() const {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		return classList.find(typeid(T).hash_code()) != classList.end();
	}
	template<typename T>
	bool hasType(T* t) const {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		return classList.find(typeid(*t).hash_code()) != classList.end();
	}


	template<typename T>
	std::unique_ptr<T> getUniqueItem(std::string const& _name) const {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		for (auto const& base : constructorList.at(_name)) {
			auto ptr = dynamic_cast<BaseT<T> const*>(base.get());
			if (ptr != nullptr) {
				return std::unique_ptr<T> { ptr->template createUnique<T>() };
			}
		}
		throw std::runtime_error("couldn't create unique item");
	}
	template<typename T>
	std::shared_ptr<T> getSharedItem(std::string const _name) const {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		for (auto const& base : constructorList.at(_name)) {
			auto ptr = dynamic_cast<BaseT<T> const*>(base.get());
			if (ptr != nullptr) {
				return ptr->template createShared<T>();
			}
		}
		throw std::runtime_error("couldn't create shared item");
	}

	template<typename T>
	void copy(std::string const& _name, T* _dest, T const* _src) const {
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		for (auto const& base : constructorList.at(_name)) {
			auto ptr = dynamic_cast<BaseT<T> const*>(base.get());
			if (ptr != nullptr) {
				ptr->copy(_dest, _src);
				return;
			}
		}
	}
#ifdef BUSY_SERIALIZER
	template<typename BASE, typename Node>
	void serialize(BASE* _base, Node& _node) {
		auto _name = getType(_base);
		for (auto const& base : constructorList.at(_name)) {
			auto ptr = dynamic_cast<BaseT<BASE> const*>(base.get());
			if (ptr != nullptr) {
				ptr->serialize(_base, _node);
				return;
			}
		}
		throw std::runtime_error("couldn't serialize item");

	}
#endif


};


template<typename T, typename ...Bases>
class Register {
	std::string mName;
public:
	Register(std::string const& _name)
		: mName(_name)
	{
		staticAssertClass<T, Bases...>();
		GenericFactory::getInstance().registerClass<T, Bases...>(mName);
	}
	~Register() {
		GenericFactory::getInstance().unregisterClass<T, Bases...>(mName);
	}
private:
	template<typename T2>
	void staticAssertClass() const {
		static_assert(std::is_polymorphic<T2>::value, "must be polymorphic type");
		static_assert(std::has_virtual_destructor<T2>::value, "must have a virtual destructor");
	}
	template<typename T2, typename BASE, typename ...Bases2>
	void staticAssertClass() const {
		staticAssertClass<BASE, Bases2...>();
		static_assert(std::is_polymorphic<BASE>::value, "must be polymorphic type");
		static_assert(std::is_polymorphic<T2>::value, "must be polymorphic type");
		static_assert(std::is_base_of<BASE, T>::value, "Base must be base of T");
		static_assert(std::has_virtual_destructor<T2>::value, "must have a virtual destructor");
	}


};


template<typename T, typename std::enable_if<std::is_polymorphic<T>::value>::type* = nullptr>
inline std::set<std::string> getClassList() {
	return GenericFactory::getInstance().getValidNames<T>();
}
template<typename T, typename std::enable_if<not std::is_polymorphic<T>::value>::type* = nullptr>
inline std::set<std::string> getClassList() {
	throw std::runtime_error("this should not happen (genericFactory");
}
template<typename T>
inline bool hasType(T* t) {
	return GenericFactory::getInstance().hasType(t);
}
template<typename T>
inline std::string const& getTypeName(T* t) {
	return GenericFactory::getInstance().getType(t);
}


template<typename T, typename std::enable_if<std::is_polymorphic<T>::value>::type* = nullptr>
inline std::shared_ptr<T> make_shared(std::string const& _name) {
	return GenericFactory::getInstance().getSharedItem<T>(_name);
}
template<typename T, typename std::enable_if<not std::is_polymorphic<T>::value>::type* = nullptr>
inline std::shared_ptr<T> make_shared(std::string const& _name) {
	throw std::runtime_error("this should not happen (genericFactory");
}

template<typename T, typename std::enable_if<std::is_polymorphic<T>::value>::type* = nullptr>
inline std::unique_ptr<T> make_unique(std::string const& _name) {
	return GenericFactory::getInstance().getUniqueItem<T>(_name);
}
template<typename T, typename std::enable_if<not std::is_polymorphic<T>::value>::type* = nullptr>
inline std::unique_ptr<T> make_unique(std::string const& _name) {
	throw std::runtime_error("this should not happen (genericFactory");
}

template<typename T>
inline std::shared_ptr<T> make_shared(T* _t) {
	auto type = GenericFactory::getInstance().getType(_t);
	return make_shared<T>(type);
}
template<typename T>
inline std::unique_ptr<T> make_unique(T* _t) {
	auto type = GenericFactory::getInstance().getType(_t);
	return make_unique<T>(type);
}

template<typename T>
inline std::unique_ptr<T> copy_unique(T* _t) {
	auto type = GenericFactory::getInstance().getType(_t);
	auto ptr = make_unique<T>(type);
	GenericFactory::getInstance().copy(type, ptr.get(), _t);
	return ptr;
}

template <typename T>
inline void activateRegisterRecorder() {
	GenericFactory::getInstance().activateRegisterRecorder<T>();
}
inline auto deactivateRegisterRecorder() -> std::vector<std::string> {
	return GenericFactory::getInstance().deactivateRegisterRecorder();
}

#ifdef BUSY_SERIALIZER
template<typename BASE, typename Node>
inline void serialize(BASE* _base, Node& _node) {
	GenericFactory::getInstance().serialize(_base, _node);
}
#endif



}

