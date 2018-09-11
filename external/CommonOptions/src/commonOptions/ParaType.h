#pragma once

#include <cstdint>
#include <list>
#include <set>
#include <string>
#include <vector>

namespace commonOptions {

enum class ParaType { None, One };


template<typename T>
struct ParaTypeInfo {
	static ParaType type() {
		return ParaType::One;
	}
};

template<typename T>
ParaType getParaType() {
	return ParaTypeInfo<T>::type();
}

}
