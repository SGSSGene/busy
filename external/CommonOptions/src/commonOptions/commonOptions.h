#pragma once

#include "Section.h"
#include "Singleton.h"
#include "Switch.h"
#include "utils.h"

namespace commonOptions {

bool& hasError();
void  print();
void  printShellCompl();

bool parse(int argc, char const* const* argv);

void loadFile(std::string const& _file);
void saveFile(std::string const& _file, std::vector<std::string> const& _includingSections);

Section* get_section(std::string const& _str);

Section* getRootSection();

template <typename ...Args>
auto make_option(Args const&... args) -> decltype(getRootSection()->make_option(args...)) {
	return getRootSection()->make_option(args...);
}

inline auto make_switch(std::string const& _str, std::string const& _description) -> Switch& {
	return getRootSection()->make_switch(_str, _description);
}
inline auto make_command(std::string const& _str, std::string const& _description) -> Switch& {
	return getRootSection()->make_switch("__command__" + _str, _description);
}
template <typename ...Args>
auto make_command(std::string const& _str, Args const&... args) -> decltype(getRootSection()->make_option(_str, args...)) {
	return getRootSection()->make_option("__command__" + _str, args...);
}
inline auto make_section(std::string const& _str) -> decltype(get_section(_str)) {
	return get_section(_str);
}
inline auto register_on_before_save(std::function<void()> _callback) -> decltype (Singleton::getInstance().register_on_before_save(_callback)) {
	return Singleton::getInstance().register_on_before_save(_callback);
}
inline auto register_on_save(std::function<void()> _callback) -> decltype (Singleton::getInstance().register_on_save(_callback)) {
	return Singleton::getInstance().register_on_save(_callback);
}
inline auto register_on_load(std::function<void()> _callback) -> decltype (Singleton::getInstance().register_on_load(_callback)) {
	return Singleton::getInstance().register_on_load(_callback);
}

inline void signal_save() {
	Singleton::getInstance().signal_save();
}


inline auto get_option(std::string const& _str) -> BaseOption* {
	return getRootSection()->getVariable(_str);
}
inline auto get_description(std::string const& _str) -> OptionDescription* {
	return getRootSection()->getDescription(_str);
}
inline bool has_key(std::string const& _str) {
	return getRootSection()->hasKey(_str);
}

auto getUnmatchedParameters() -> std::vector<std::string>;

}
