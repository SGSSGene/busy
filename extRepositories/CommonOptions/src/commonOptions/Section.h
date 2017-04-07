#pragma once


#include "Switch.h"
#include "utils.h"

#include <map>
#include <string>

namespace commonOptions {

class Section {
private:
	Section*    mParent;
	std::string mName;
	std::map<std::string, Section> mChildren;

	std::map<std::string, std::unique_ptr<BaseOption>> mVariables;
	std::map<std::string, std::unique_ptr<OptionDescription>> mDescriptions;
public:
	Section();
	Section(Section const& _other);
	Section(Section* _parent, std::string const& _name);

	Section& operator=(Section const& _other);

	auto getVariables() -> std::vector<BaseOption*>;
	auto getChildren() const -> std::map<std::string, Section> const&;
	auto getChildren() -> std::map<std::string, Section>&;
	auto getDescriptions() const -> std::map<std::string, std::unique_ptr<OptionDescription>> const&;

	auto getAllDescriptions() const -> std::map<std::string, OptionDescription const*>;

private:
	void getVariablesImpl(std::vector<BaseOption*>* options);
	void getAllDescriptionsImpl(std::map<std::string, OptionDescription const*>& _descriptions) const;

public:
	auto getVariable(std::string const& _name) -> BaseOption*;
	auto getDescription(std::string const& _name) -> OptionDescription*;
	bool hasKey(std::string const& _name);

	Section* accessChild(std::string const& _name);

	std::string fullName() const;

	auto getSectionOfVariable(std::string const& _str) -> std::pair<Section*, std::string> {
		auto path = splitPath(_str);
		auto variableName = path.back();
		path.pop_back();
		Section* section = this;
		for (auto const& p : path) {
			section = section->accessChild(p);
		}
		return {section, variableName};
	}

	template<typename T>
	auto make_option(std::string const& _str, T _default, std::vector<T> _selection, std::string const& _description) -> Option<T>& {
		auto v = getSectionOfVariable(_str);
		if (v.first != this) {
			return v.first->make_option(v.second, _default, std::move(_selection), _description);
		}
		if (mVariables.find(v.second) == mVariables.end()) {
			mVariables[v.second].reset(new Option<T>(this, v.second, _default, std::move(_selection), _description));
		}
		return dynamic_cast<Option<T>&>(*mVariables.at(v.second));
	}
	auto make_option(std::string const& _str, char const* _default, std::vector<std::string> _selection, std::string const& _description) -> Option<std::string>& {
		return make_option<std::string>(_str, _default, std::move(_selection), _description);
	}


	template<typename T>
	auto make_option(std::string const& _str, T _default, std::string const& _description) -> Option<T>& {
		auto v = getSectionOfVariable(_str);
		if (v.first != this) {
			return v.first->make_option(v.second, _default, _description);
		}
		if (mVariables.find(v.second) == mVariables.end()) {
			auto ptr = new Option<T>(this, v.second, _default, _description);
			mVariables[v.second].reset(ptr);
		}
		return dynamic_cast<Option<T>&>(*mVariables.at(v.second));
	}
	auto make_option(std::string const& _str, char const* _default, std::string const& _description) -> Option<std::string>& {
		return make_option<std::string>(_str, _default, _description);
	}

	auto make_switch(std::string const& _str, std::string const& _description) -> Switch& {
		auto v = getSectionOfVariable(_str);
		if (v.first != this) {
			return v.first->make_switch(v.second, _description);
		}
		if (mVariables.find(v.second) == mVariables.end()) {
			mVariables[v.second].reset(new Switch(this, v.second, _description));
		}
		return dynamic_cast<Switch&>(*mVariables.at(v.second));
	}
};

}
