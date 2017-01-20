#include <stdlib.h>
#include <stdio.h>
#include <haste/panic>

namespace haste {

void panic(const char* message) {
	fprintf(::stderr, "panic: %s\n", message);
	::abort();
}

void warn(const char* message) {
	fprintf(::stderr, "warning: %s\n", message);
}

}
