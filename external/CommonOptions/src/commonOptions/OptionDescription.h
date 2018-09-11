#pragma once


#include <functional>
#include <string>


namespace commonOptions {

/**
 * Description of an option
 */
struct OptionDescription {
	std::string optionName;
	std::string description;
	int         defaultValueLevel;
	std::string defaultValue;
	bool        defaultValueActive;
	std::string value;

	std::function<void()> onDefaultValueChange;
	std::function<void()> onValueChange;

	void changeDefaultValue(std::string const& _defaultValue, int _level);
	void changeValue(std::string const& _value);
};

}
