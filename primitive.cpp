#include <haste/primitive>
#include <haste/unittest>

namespace haste {

static_assert(sizeof(i8) == 1, "");
static_assert(sizeof(i16) == 2, "");
static_assert(sizeof(i32) == 4, "");
static_assert(sizeof(i64) == 8, "");

static_assert(sizeof(u8) == 1, "");
static_assert(sizeof(u16) == 2, "");
static_assert(sizeof(u32) == 4, "");
static_assert(sizeof(u64) == 8, "");

static_assert(sizeof(f32) == 4, "");
static_assert(sizeof(f64) == 8, "");

// is_iterable

struct uniterable {};

static_assert(!is_iterable<uniterable>, "");

struct iterable {
  iterable& begin();
  iterable& end();
  int operator*() const;
  void operator++();
  bool operator!=(const iterable&) const;
};

struct sized_iterable {
  iterable& begin();
  iterable& end();
  int operator*() const;
  void operator++();
  bool operator!=(const iterable&) const;
  int size() const;
};

struct unsized_iterable {
  iterable& begin();
  iterable& end();
  int operator*() const;
  void operator++();
  bool operator!=(const iterable&) const;
  void size() const;
};

static_assert(is_iterable<iterable>, "");
static_assert(!has_size<iterable>, "");
static_assert(has_size<sized_iterable>, "");
static_assert(!has_size<unsized_iterable>, "");

static_assert(is_iterable<std::initializer_list<int>> &&
                  has_size<std::initializer_list<int>>,
              "");

class test {
public:
  template <class I>
  test(I&&, enable_if_sized_iterable<I> = 0) {}

  test(std::initializer_list<int> x) : test(x, 0) { }


};

unittest() {


  auto x = test({1, 2, 3});
  x = x;
}
}
