#include <haste/unittest>
#include <haste/string_view>
#include <cstring>

namespace haste {

void string_view::_zstring(const char* str) {
  _begin = str;
  _end = _begin + std::strlen(str);
}

/*unittest() {
  string_view();
  string_view("");
  string_view("foo");
  string_view(nullptr);

  const char* _0 = "foo";
  string_view _1(_0);

  const char* _2 = nullptr;
  string_view _3(_2);
}*/

}
