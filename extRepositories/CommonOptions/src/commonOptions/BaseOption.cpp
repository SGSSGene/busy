#include "BaseOption.h"
#include "Section.h"

#include "OptionDescription.h"
#include "ParaType.h"

namespace commonOptions {

BaseOption::BaseOption(Section* _section, std::string const& _name, ParaType _paraType)
	: mSection (_section)
	, mName (_name)
	, mParaType (_paraType)
	, mOptionDescription ( nullptr )
{
}

BaseOption::~BaseOption() {
}

auto BaseOption::getSectionName() const -> std::string {
	return mSection->fullName();
}

auto BaseOption::getName() const -> std::string const& {
	return mName;
}
auto BaseOption::getParaType() const -> ParaType {
	return mParaType;
}
void BaseOption::createDescription(std::string const& _defaultValue, std::string const& _description) {
	//!TODO handling of changing default value or description
	mOptionDescription = mSection->getDescription(mName);
	if (mOptionDescription->description != "" and _description == ""
	    and mOptionDescription->description != _description) {
		throw std::runtime_error("description of option is being changed, not a good idea");
	}
	if (_description != "") {
		mOptionDescription->description = _description;
	}
	if (mOptionDescription->defaultValueLevel <= 0) {
		mOptionDescription->defaultValue = _defaultValue;
		mOptionDescription->defaultValueLevel = 0;
	}
}
void BaseOption::print() const {
	bool isCommand = mName.find("__command__") == 0;

	std::stringstream ss;
	if (not isCommand) {
		ss<<"--";
		ss<<mSection->fullName();
		ss<<mOptionDescription->optionName;
	} else {
		ss<<mSection->fullName();
		ss << mOptionDescription->optionName.c_str() + std::string("__command__").size();
	}
	if (mParaType != ParaType::None) {
		ss<<" "<<mOptionDescription->defaultValue;
	}
	while(ss.str().length() < 42) {
		ss<<" ";
	}
	ss<<mOptionDescription->description;
	std::cout<<ss.str()<<std::endl;
}
void BaseOption::printShellCompl() const {
	bool isCommand = mName.find("__command__") == 0;
	if (not isCommand) {
		std::cout << "--";
		std::cout << mSection->fullName();
		std::cout << mOptionDescription->optionName << " ";
	} else {
		std::cout << mSection->fullName();
		std::cout << mOptionDescription->optionName.c_str() + std::string("__command__").size() << " ";
	}
}



}
