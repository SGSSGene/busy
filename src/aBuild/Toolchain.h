#pragma once

#include <jsonSerializer/jsonSerializer.h>

namespace aBuild {

class Toolchain {
private:
	std::string name;
	std::vector<std::string> cCompiler;
	std::vector<std::string> cppCompiler;
	std::vector<std::string> archivist;

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
	auto getCCompiler() const -> std::vector<std::string> {
		return cCompiler;
	}
	auto getCppCompiler() const -> std::vector<std::string> {
		return cppCompiler;
	}
	auto getArchivist() const -> std::vector<std::string> {
		return archivist;
	}
	void serialize(jsonSerializer::Node& node) {
		node["name"]                  % name;
		node["ccompiler"]             % cCompiler;
		node["cppcompiler"]           % cppCompiler;
		node["archivist"]             % archivist;
	}

};

}
