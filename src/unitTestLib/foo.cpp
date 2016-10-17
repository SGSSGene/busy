#include "foo.h"
#include <unitTestStaticLib/bar.h>

int foo() {
	return bar() - 1;
}
