#include <haste/os>
#include <haste/unittest>

#include <errno.h>
#include <unistd.h>

namespace haste {

/* expected<string, os_error> getcwd() {
  auto result = list<char>::uninitialized_inplace_list();

  while (::getcwd(result.data(), result.size()) == NULL) {
    switch (errno) {
      case EACCES: return os_error::access_denied;
      case ENOENT: return os_error::directory_unlinked;
      case ENOMEM: return os_error::out_of_memory;
      default: return os_error::unknown_error;
    }

    result = result.extend();
  }

  return string(move(result)).rstrip_c_str();
}*/

/*
unittest() {
  assert_eq(getcwd().ignore(), "");

  if (auto x = getcwd()) {
  }
}

unittest() {
  chdir("test/listdir", [] {

    listdir("empty");

  });
}*/
}
