#include "OptionDescription.h"

#include <iostream>

namespace commonOptions {

void OptionDescription::changeDefaultValue(std::string const& _defaultValue, int _level) {
	if (_level < defaultValueLevel) return;
	defaultValueLevel = _level;
	defaultValue = _defaultValue;
	if (onDefaultValueChange) {
		onDefaultValueChange();
	}
}
void OptionDescription::changeValue(std::string const& _value) {
	value = _value;
	if (onValueChange) {
		onValueChange();
	}
}

}
