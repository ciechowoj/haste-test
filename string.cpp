#include <string.h>
#include <haste/string>
#include <haste/unittest>

namespace haste {

void string_view::_zstring(const char* str) {
  _begin = str;
  _end = _begin + ::strlen(str);
}

unittest() {
  string_view();
  string_view("");
  string_view("foo");
  string_view(nullptr);

  const char* _0 = "foo";
  string_view _1(_0);

  const char* _2 = nullptr;
  string_view _3(_2);
}

/*unittest() {

        string("{}").format(13);

        "{}"_format(123);

        // format("{}", 13);
        // parse("123");



        // i32 x = "{}"_format(123).i32();






}*/
}
