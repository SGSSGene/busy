#include "Section.h"
#include "commonOptions.h"

namespace commonOptions {

Section::Section()
	: mParent(nullptr)
{
}
Section::Section(Section const& _other) {
	*this = _other;
}

Section::Section(Section* _parent, std::string const& _name)
	: mParent (_parent)
	, mName   (_name)
{
}

Section& Section::operator=(Section const& _other) {
	mParent = _other.mParent;
	mName   = _other.mName;
	return *this;
}

auto Section::getVariables() -> std::vector<BaseOption*> {
	std::vector<BaseOption*> options;
	getVariablesImpl(&options);
	return options;
}
auto Section::getChildren() const -> std::map<std::string, Section> const& {
	return mChildren;
}
auto Section::getChildren() -> std::map<std::string, Section>& {
	return mChildren;
}
auto Section::getDescriptions() const -> std::map<std::string, std::unique_ptr<OptionDescription>> const& {
	return mDescriptions;
}

auto Section::getAllDescriptions() const -> std::map<std::string, OptionDescription const*> {
	std::map<std::string, OptionDescription const*> descriptions;
	getAllDescriptionsImpl(descriptions);
	return descriptions;
}

void Section::getVariablesImpl(std::vector<BaseOption*>* options) {
	for (auto& p : mVariables) {
		options->push_back(p.second.get());
	}
	for (auto& child : mChildren) {
		child.second.getVariablesImpl(options);
	}
}
void Section::getAllDescriptionsImpl(std::map<std::string, OptionDescription const*>& _descriptions) const {
	for (auto const& d : mDescriptions) {
		_descriptions[fullName() + d.first] = d.second.get();
	}
	for (auto const& child : mChildren) {
		child.second.getAllDescriptionsImpl(_descriptions);
	}
}

auto Section::getVariable(std::string const& _name) -> BaseOption* {
	auto p = getSectionOfVariable(_name);
	if (p.first != this) {
		return p.first->getVariable(p.second);
	}

	if (mVariables.find(_name) != mVariables.end()) {
		return mVariables.at(_name).get();
	}

	return nullptr;
}

auto Section::getDescription(std::string const& _name) -> OptionDescription* {
	auto p = getSectionOfVariable(_name);
	if (p.first != this) {
		return p.first->getDescription(p.second);
	}

	if (mDescriptions.find(_name) == mDescriptions.end()) {
		auto& ptr = mDescriptions[_name];
		ptr.reset(new OptionDescription);
		ptr->optionName = _name;
		ptr->defaultValueActive = true;
		ptr->defaultValueLevel  = -1;
	}
	return mDescriptions.at(_name).get();
}
bool Section::hasKey(std::string const& _name) {
	auto p  = getSectionOfVariable(_name);
	if (p.first != this) {
		return p.first->hasKey(p.second);
	}
	return mVariables.find(_name) != mVariables.end();
}




Section* Section::accessChild(std::string const& _name) {
	auto iter = mChildren.find(_name);
	if (iter == mChildren.end()) {
		iter = mChildren.insert({_name, Section(this, _name)}).first;
	}
	return &iter->second;
}

std::string Section::fullName() const {
	if (not mParent) return "";
	return mParent->fullName() + mName + ".";
}



}
