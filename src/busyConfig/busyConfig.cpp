#include "busyConfig.h"
#include <serializer/serializer.h>

namespace busyConfig {
	auto readPackage(std::string const& _path) -> Package {
		Package package;
		serializer::yaml::read(_path + "/busy.yaml" , package);
		return package;

	}

}
