#include <haste/list>
#include <haste/range>
#include <haste/unittest>

namespace haste {

unittest() {
  int result[16];
  usize i = 0;

  for (auto&& x : range<int>(0, 16)) {
    result[i++] = x;
  }

  for (int i = 0; i < int(sizeof(result) / sizeof(result[0])); ++i) {
    assert_eq(result[i], i);
  }
}

}
