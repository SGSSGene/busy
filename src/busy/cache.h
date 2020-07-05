#pragma once

#include "finally.h"

namespace busy {

void loadFileCache();
void saveFileCache(bool _yamlCache);

auto loadFileCache(bool _yamlCache) -> finally;

}
