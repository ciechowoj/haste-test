#include <string.h>
#include <haste/string>
#include <haste/unittest>

namespace haste {

string string::rstrip_c_str() && {
  auto view = this->view();

  if (auto zero = ::memchr(view.data(), 0, view.size())) {
    return slice(0, (char*)zero - view.data());
  }
  else {
    return move(*this);
  }
}

string string::rstrip_c_str() const& {
  auto view = this->view();

  if (auto zero = ::memchr(view.data(), 0, view.size())) {
    return slice(0, (char*)zero - view.data());
  }
  else {
    return *this;
  }
}



unittest() {

  auto s0 = string();
  auto s1 = string("Hello world!");

  assert_eq(s1.size(), 12u);
  assert_eq(s1.data()[0], 'H');




}



}
