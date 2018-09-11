#pragma once

#include "Option.h"

namespace commonOptions {

class Switch : public Option<bool> {
public:
	Switch(Section* _section, std::string const& _varName, std::string const& _description)
		: Option(_section, _varName, false, _description) {
		mParaType = ParaType::None;

		auto _name = getSectionName() + _varName;
	}

	operator bool() {
		return **this;
	}
};


}
