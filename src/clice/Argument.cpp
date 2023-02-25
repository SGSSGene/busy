#include "Argument.h"

namespace clice {

ArgumentBase::ArgumentBase(ArgumentBase* parent)
    : parent{parent}
{
    if (parent) {
        parent->arguments.push_back(this);
    } else {
        Register::getInstance().arguments.push_back(this);
    }
}
ArgumentBase::~ArgumentBase() {
    if (parent) {
        auto& arguments = parent->arguments;
        arguments.erase(std::remove(arguments.begin(), arguments.end(), this), arguments.end());
    } else {
        auto& arguments = Register::getInstance().arguments;
        arguments.erase(std::remove(arguments.begin(), arguments.end(), this), arguments.end());
    }
}

}
