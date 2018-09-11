#pragma once

#include "binary/Serializer.h"
#include "json/Serializer.h"
#include "yaml/Serializer.h"

#include "standardTypes.h"

#ifdef BUSY_UNITS
	#include <units/units.h>
	#include "unitTypes.h"
#endif
#ifdef BUSY_ARMADILLO_BITS
	#include <armadillo>
	#include "armaTypes.h"
#endif

#include "is_serializable.h"
