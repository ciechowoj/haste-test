#include <string.h>
#include <haste/list>
#include <haste/unittest>
// #include <new>
// #include <initializer_list>

namespace haste {
namespace detail {

inline usize make_size(usize size) {
  return usize(1) << (sizeof(usize) * 8 - 1) | size;
}

inline void reset(u8* data) { *((usize*)(data - sizeof(usize))) = 1; }

inline void acquire(u8* data) {
  __atomic_add_fetch((usize*)(data - sizeof(usize)), 1, __ATOMIC_SEQ_CST);
}

inline usize release(u8* data) {
  return __atomic_sub_fetch((usize*)(data - sizeof(usize)), 1,
                            __ATOMIC_SEQ_CST);
}

trivial_list::trivial_list(usize item_size, allocator&& allocator)
    : trivial_list(0, item_size, move(allocator)) {}

trivial_list::trivial_list(usize list_size, usize item_size)
    : trivial_list(list_size, item_size, allocator()) {}

trivial_list::trivial_list(usize list_size, usize item_size,
                           allocator&& allocator) {
  auto num_bytes = list_size * item_size;

  if (allocator != haste::allocator()) {
    _allocator() = move(allocator);
    _data = (u8*)&_offset;
  } else {
    _data = (u8*)&_allocator_storage;
  }

  usize capacity = (u8*)(this + 1) - _data;

  if (capacity < num_bytes) {
    _data = (u8*)_allocator().alloc(num_bytes + sizeof(usize)) + sizeof(usize);
    _offset = 0;
    _size = make_size(list_size);
    reset(_data);
  }
}

trivial_list::~trivial_list() {
  if (_heap() && !release(_data)) {
    _allocator().free(_data - sizeof(usize));
    _allocator().~allocator();
  } else if (_data == (u8*)&_offset) {
    _allocator().~allocator();
  }
}

trivial_list::trivial_list(trivial_list&& that) {
  ::memcpy(this, &that, sizeof(trivial_list));
  ::memset(&that, 0, sizeof(trivial_list));
}

trivial_list::trivial_list(const trivial_list&) {}

trivial_list& trivial_list::operator=(trivial_list&&) { return *this; }

trivial_list& trivial_list::operator=(const trivial_list&) { return *this; }

default_list::default_list(usize list_size, usize item_size)
    : default_list(list_size, item_size, allocator()) {}

default_list::default_list(usize list_size, usize item_size,
                           allocator&& allocator) {
  auto num_bytes = list_size * item_size;
  _allocator = move(allocator);
  _data = (u8*)_allocator.alloc(num_bytes + sizeof(usize)) + sizeof(usize);
  _offset = 0;
  _size = list_size;
}
}

unittest("Current list implementation supports only little endian machines.") {
  union {
    u32 i;
    char c[4];
  } test = {0x01020304};

  assert_eq(test.c[0], 4);
}

unittest("Empty list of trivials should have zero size.") {
  auto l0 = list<int>();
  assert_eq(l0.size(), 0u);
}

unittest("Empty list of trivials should have representation of zeros.") {
  char zeros[sizeof(list<int>)] = {0};
  auto l1 = list<int>();
  assert_eq(::memcmp(&l1, zeros, sizeof(list<int>)), 0);
}

unittest("Create lists of different lengths with default no allocator.") {
  auto l0 = list<int>{};
  auto l1 = list<int>{1};
  auto l2 = list<int>{1, 2};
  auto l3 = list<int>{1, 2, 3};
  auto l4 = list<int>{1, 2, 3, 4};
  auto l5 = list<int>{1, 2, 3, 4, 5};
  auto l6 = list<int>{1, 2, 3, 4, 5, 6};
  auto l7 = list<int>{1, 2, 3, 4, 5, 6, 7};
  auto l8 = list<int>{1, 2, 3, 4, 5, 6, 7, 8};
  auto l9 = list<int>{1, 2, 3, 4, 5, 6, 7, 8, 9};
}

unittest("Create lists of different lengths with default allocator.") {
  auto l0 = list<int>(allocator());
  auto l1 = list<int>(allocator(), {1});
  auto l2 = list<int>(allocator(), {1, 2});
  auto l3 = list<int>(allocator(), {1, 2, 3});
  auto l4 = list<int>(allocator(), {1, 2, 3, 4});
  auto l5 = list<int>(allocator(), {1, 2, 3, 4, 5});
  auto l6 = list<int>(allocator(), {1, 2, 3, 4, 5, 6});
  auto l7 = list<int>(allocator(), {1, 2, 3, 4, 5, 6, 7});
  auto l8 = list<int>(allocator(), {1, 2, 3, 4, 5, 6, 7, 8});
  auto l9 = list<int>(allocator(), {1, 2, 3, 4, 5, 6, 7, 8, 9});
}

unittest("Create lists of different lengths with explicit allocator.") {
  auto l0 = list<int>(explicit_allocator());
  auto l1 = list<int>(explicit_allocator(), {1});
  auto l2 = list<int>(explicit_allocator(), {1, 2});
  auto l3 = list<int>(explicit_allocator(), {1, 2, 3});
  auto l4 = list<int>(explicit_allocator(), {1, 2, 3, 4});
  auto l5 = list<int>(explicit_allocator(), {1, 2, 3, 4, 5});
  auto l6 = list<int>(explicit_allocator(), {1, 2, 3, 4, 5, 6});
  auto l7 = list<int>(explicit_allocator(), {1, 2, 3, 4, 5, 6, 7});
  auto l8 = list<int>(explicit_allocator(), {1, 2, 3, 4, 5, 6, 7, 8});
  auto l9 = list<int>(explicit_allocator(), {1, 2, 3, 4, 5, 6, 7, 8, 9});
}

unittest("Test different syntaxes for list initialization.") {
  auto l0 = list<int>();
  auto l1 = list<int>({});
  auto l2 = list<int>{};

  auto l3 = list<int>({1});
  auto l4 = list<int>{1};

  list<int> l5 = {};
  list<int> l6 = {1};
  list<int> l7 = {1, 2};

  list<list<int>> l8 = {{1, 2}, {3, 4}};
}

/*unittest("Test special functions of list of primitive type.") {
  auto l0 = list<int>();
  auto l1 = l0;
  auto l2 = move(l0);

  assert_eq(l0.size(), 0u);
  assert_eq(l1.size(), 0u);
  assert_eq(l2.size(), 0u);

  auto l3 = list<int>(1);
  auto l4 = l3;
  auto l5 = move(l3);

  assert_eq(l3.size(), 1u);
  assert_eq(l4.size(), 1u);
  assert_eq(l5.size(), 1u);

  auto l6 = list<int>(explicit_allocator(), 1, 2, 3, 4, 5, 6, 7, 8);
  auto l7 = l6;
  auto l8 = move(l6);

  assert_eq(l3.size(), 8u);
  assert_eq(l4.size(), 8u);
  assert_eq(l5.size(), 8u);
}*/
}
