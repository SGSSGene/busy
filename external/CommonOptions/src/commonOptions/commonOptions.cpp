#include "commonOptions.h"
#include "Option.h"

namespace commonOptions {

bool& hasError() {
	static bool error{false};
	return error;
}


void print() {
	auto allOptions = getRootSection()->getVariables();
	for (auto b : allOptions) {
		b->print();
	}
}
void printShellCompl() {
	auto allOptions = getRootSection()->getVariables();
	for (auto b : allOptions) {
		b->printShellCompl();
	}
}

/**
 * This is a very simple command line parser
 * It can parse stuff like
 * --option
 * --option value
 * --opiton=value
 *
 *  it will ignore everything else on the command line. This is needed because boost::
 *  program_options is handling unknown options very badly
 */
bool parse(int argc, char const* const* argv) {
	std::map<std::string, std::string> options;

	for (int i(1); i<argc; ++i) {
		std::string arg = argv[i];
		if (0 == arg.compare(0, 2, "--")) {
			arg = arg.substr(2); // cut of first two symbols
		} else {
			arg = "__command__" + arg;
		}
		std::string key   = arg;
		std::string value = "true";

		size_t equalSignPos = arg.find("=");
		if (equalSignPos != std::string::npos) {
			key   = arg.substr(0, equalSignPos);
			value = arg.substr(equalSignPos+1);
		} else if (has_key(key) and get_option(key)->isListType()) {
			if (i+1 < argc) {
				value = argv[++i];
				while (i+1 < argc) {
					value += std::string(", ") + argv[++i];
				}
			}
			value = "[" + value + "]";
		} else if (i+1 < argc && std::string(argv[i+1]).compare(0, 2, "--" ) != 0) {
			value = argv[++i];
		}
		auto description = get_description(key);
		description->changeDefaultValue(value, 1);
	}
/*	if (commands.size() >= 1) {
		auto cmdDescription = get_description("__command__" + commands[0]);
		cmdDescription->changeDefaultValue("true", 1);

		commands.erase(commands.begin());
		auto filesDescription = get_description("__files__");
		filesDescription->changeDefaultValue(serializer::yaml::writeAsString(commands), 1);
	}*/

	if (argc == 2 && std::string(argv[1]) == "__completion") {
		printShellCompl();
		exit(0);
	}

	return not hasError();
}

void loadFile(std::string const& _file) {
	std::map<std::string, std::string> options;

	serializer::yaml::read(_file, options);

	for (auto o : options) {
		auto desc = get_description(o.first);
		desc->changeValue(o.second);
		desc->defaultValueActive = false;
	}
	Singleton::getInstance().signal_load();
}

/** if _includingSections is empty, include them all
 */
void saveFile(std::string const& _file, std::vector<std::string> const& _includingSections) {
	std::map<std::string, std::string> options;

	std::set<Section*> allSections;

	std::queue<Section*> sectionsToProcess;

	// collecting all sections (to remove duplicates)
	for (auto const& sectName : _includingSections) {
		sectionsToProcess.push(get_section(sectName));
	}
	if (_includingSections.empty()) {
		sectionsToProcess.push(getRootSection());
	}
	while (not sectionsToProcess.empty()) {
		auto sect = sectionsToProcess.front();
		sectionsToProcess.pop();
		allSections.insert(sect);
		for (auto& child : sect->getChildren()) {
			sectionsToProcess.push(&child.second);
		}
	}

	// Serialize all OptionDescriptions (if they don't have default value)
	for (auto section : allSections) {
		auto fullName = section->fullName();

		for (auto const& description : section->getDescriptions()) {
			auto name = fullName + description.second->optionName;
			if (not description.second->defaultValueActive) {
				auto const* d = description.second.get();
				// TODO hack, if value looks like default value, don't save it
				if (description.second->defaultValue != d->value) {
					options[name] = d->value;
				}
			}
		}
	}
	serializer::yaml::write(_file, options);
}

Section* get_section(std::string const& _str) {
	auto path = splitPath(_str);
	Section* section = getRootSection();
	for (auto const& p : path) {
		section = section->accessChild(p);
	}
	return section;
}


Section* getRootSection() {
	static Section singleton;
	return &singleton;
}

auto getUnmatchedParameters() -> std::vector<std::string> {
	std::vector<std::string> missing;

	std::set<std::string> available;
	auto variables = commonOptions::getRootSection()->getVariables();
	for (auto v : variables) {
		available.insert(v->getSectionName() + v->getName());
	}

	auto descriptions = commonOptions::getRootSection()->getAllDescriptions();
	for (auto const& d : descriptions) {
		if (available.count(d.first) == 0) {
			missing.push_back(d.first);
		}
	}
	return missing;
}
}
