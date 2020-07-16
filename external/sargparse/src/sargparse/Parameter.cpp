#include "Parameter.h"

#include <algorithm>


namespace sargp {

Task::Task(Callback cb, Command& command)
	:_command{command}, _cb{std::move(cb)}
{
	_command.registerTask(*this);
}
Task::~Task() {
	_command.deregisterTask(*this);
}


ParameterBase::ParameterBase(std::string argName, DescribeFunc describeFunc, Callback cb, ValueHintFunc hintFunc, Command& command, std::type_info const& _type_info)
	: _argName{std::move(argName)}
	, _describeFunc{std::move(describeFunc)}
	, _cb{std::move(cb)}
	, _hintFunc{std::move(hintFunc)}
	, _command{command}
	, type_info{_type_info}
{
	_command.registerParameter(*this);
}

ParameterBase::~ParameterBase() {
	_command.deregisterParameter(*this);
}

Command::Command(Command* parentCommand, std::string name, std::string description, Callback cb)
	: _name(std::move(name))
	, _description(std::move(description))
	, _tasks{}
	, _defaultTask{std::make_unique<Task>(move(cb), *this)}
	, _parentCommand{parentCommand?nullptr:&Command::getDefaultCommand()}
{
	_parentCommand->subcommands.emplace_back(this);
}

Command::Command(DefaultCommand)
{}



Command& Command::getDefaultCommand() {
	static Command instance{Command::DefaultCommand{}};
	return instance;
}


void Command::callCBs() const {
	for (auto task : _tasks) {
		(*task)();
	}
}

Command& getDefaultCommand() {
	return Command::getDefaultCommand();
}





}
