#include <stdio.h>
#include <stdlib.h>
#include <haste/panic>

namespace haste {

void panic(const char* message) {
  fprintf(::stderr, "panic: %s\n", message);
  ::abort();
}

void panic(panic_reason reason) {
  switch (reason) {
    case panic_reason::index_error:
      panic("index error");
      break;
    default:
      panic("unknown error");
      break;
  }
}

void warn(const char* message) { fprintf(::stderr, "warning: %s\n", message); }
}
