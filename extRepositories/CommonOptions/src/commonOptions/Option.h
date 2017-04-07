#pragma once

#include "ParaType.h"
#include "BaseOption.h"

#include <serializer/serializer.h>

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

namespace commonOptions {

class Section;

template<typename T>
struct CurrentValue {
	T    value;
};

template<typename ...Args>
struct testListType {
	constexpr static bool value = false;
};
template<typename T>
struct testListType<std::vector<T>> {
	constexpr static bool value = true;
};
template<typename T>
struct testListType<std::list<T>> {
	constexpr static bool value = true;
};



/**
 * This type must be convertable by using stringstream
 */
template<typename T>
class Option : public BaseOption {
private:
	bool                             onlyPossibleValues;
	std::vector<T>                   possibleValues;
	std::shared_ptr<CurrentValue<T>> mCurrentValue;
public:
	Option(Section* _section, std::string const& _name, T _default, std::string const& _description)
		: Option(_section, _name, _default, {}, _description) {
	}
	Option(Section* _section, std::string const& _varName, T _default, std::vector<T> _list, std::string const& _description)
		: BaseOption(_section, _varName, commonOptions::getParaType<T>())
	{
		onlyPossibleValues = not _list.empty();
		possibleValues = std::move(_list);
		possibleValues.push_back(_default);

		auto _name = getSectionName() + _varName;

		createDescription(serializer::yaml::writeAsString(_default), _description);

		mCurrentValue = std::make_shared<CurrentValue<T>>();
		if (mOptionDescription->defaultValueActive) {
			mCurrentValue->value = _default;
		} else {
			serializer::yaml::readFromString(mOptionDescription->value, mCurrentValue->value);
		}

		mOptionDescription->onDefaultValueChange = [=] {
			if (mOptionDescription->defaultValueActive) {
				serializer::yaml::readFromString(mOptionDescription->defaultValue, mCurrentValue->value);
			}
		};
		mOptionDescription->onValueChange = [=] {
			if (mOptionDescription->defaultValueActive) {
				serializer::yaml::readFromString(mOptionDescription->value, mCurrentValue->value);
			}
		};
	}

	T const* operator->() const {
		return &mCurrentValue->value;
	}

	T const& operator*() const {
		return mCurrentValue->value;
	}
	bool isListType() const override {
		return testListType<T>::value;
	}


	void resetToDefault() {
		mOptionDescription->defaultValueActive = true;
		serializer::yaml::readFromString(mOptionDescription->defaultValue, mCurrentValue->value);
	}
	void setValue(T const& t) {
		mCurrentValue->value = t;
		mOptionDescription->value = serializer::yaml::writeAsString(mCurrentValue->value);
		mOptionDescription->defaultValueActive = false;
	}
	void setValue(T&& t) {
		mCurrentValue->value = t;
		mOptionDescription->value = serializer::yaml::writeAsString(mCurrentValue->value);
		mOptionDescription->defaultValueActive = false;
	}
};

}
