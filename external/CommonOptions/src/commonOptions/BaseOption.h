#pragma once

#include "OptionDescription.h"
#include "ParaType.h"

#include <functional>
#include <string>

namespace commonOptions {

class Section;

class BaseOption {
protected:
	Section*    mSection;
	std::string mName;
	ParaType    mParaType;
	OptionDescription* mOptionDescription;
public:
	BaseOption(Section* _section, std::string const& _name, ParaType _paraType);
	virtual ~BaseOption();

	auto getSectionName() const -> std::string;
	auto getName() const -> std::string const&;
	auto getParaType() const -> ParaType;
	void createDescription(std::string const& _defaultValue, std::string const& _description);

	void print() const;
	void printShellCompl() const;

	virtual bool isListType() const { return false; }
};

}
