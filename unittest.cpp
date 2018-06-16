#include <haste/test>
#include <stdexcept>

int main() {
  return haste::run_all_tests() ? 0 : 1;
}

unittest() {
  haste::assert_true(true);
}

unittest() {
  haste::assert_throws([] { throw "exception"; });
}
