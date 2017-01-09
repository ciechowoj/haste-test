#include <haste/string_view>

#include <cstring>
#include <haste/unittest.hpp>

namespace haste {


void format()

void parse(double& , istream_view istream);



template <class... args_t>
void format(ostream_view_t stream, string_view_t format,
            const args_t&... args) {}

unittest() {
  int x;

  const char* h = "haste";

  format(x, "");
  format(x, h, 1, 2, 3, h);
}
}
