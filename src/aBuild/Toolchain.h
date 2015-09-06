#pragma once

#include <jsonSerializer/jsonSerializer.h>

namespace aBuild {

class Toolchain {
private:
	std::string name;
	std::string cCompiler;
	std::string cppCompiler;
	std::string archivist;
public:
	Toolchain() {}
	Toolchain(std::string _name, std::string _cCompiler, std::string _cppCompiler, std::string _archivist)
		: name        {_name }
		, cCompiler   { _cCompiler }
		, cppCompiler { _cppCompiler }
		, archivist   { _archivist } {
	}
	auto getName() const -> std::string {
		return name;
	}
	auto getCCompiler() const -> std::string {
		return cCompiler;
	}
	auto getCppCompiler() const -> std::string {
		return cppCompiler;
	}

	auto getArchivist() const -> std::string {
		return archivist;
	}
	void serialize(jsonSerializer::Node& node) {
		node["name"]        % name;
		node["ccompiler"]   % cCompiler;
		node["cppcompiler"] % cppCompiler;
		node["archivist"]   % archivist;
	}

};

}
