#include <haste/os>
#include <haste/unittest>

#include <errno.h>
#include <unistd.h>

namespace haste {

void chdir(string_view) {}

expected<string, os_error> getcwd() {
  string_builder builder(string_builder::default_buffer_size);

  while (::getcwd(builder.data(), builder.size()) == NULL) {
    switch (errno) {
      case EACCES: return os_error::access_denied;
      case ENOENT: return os_error::directory_unlinked;
      case ENOMEM: return os_error::out_of_memory;
      default: return os_error::unknown_error;
    }

    builder.uninitialized_extend();
  }

  return builder.string();
}

unittest() {
  assert_eq(getcwd().ignore(), "");

  if (auto x = getcwd()) {
  }
}

unittest() {
  chdir("test/listdir", [] {

    listdir("empty");

  });
}
}
