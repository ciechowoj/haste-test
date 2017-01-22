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

inline void reset(u8* data, allocator&& allocator) {
  *((usize*)(data - sizeof(usize))) = 1;
  new (data - sizeof(usize) - sizeof(allocator))
      haste::allocator(move(allocator));
}

inline void acquire(u8* data, usize offset) {
  __atomic_add_fetch((usize*)(data - sizeof(usize) - offset), 1,
                     __ATOMIC_SEQ_CST);
}

inline bool release(u8* data, usize offset) {
  return __atomic_sub_fetch((usize*)(data - sizeof(usize) - offset), 1,
                            __ATOMIC_SEQ_CST) == 0;
}

inline allocator& external_allocator(u8* data, usize offset) {
  return *((allocator*)(data - offset - sizeof(usize) - sizeof(allocator)));
}

inline u8* create_buffer(allocator&& allocator, usize num_bytes) {
  auto data =
      (u8*)allocator.alloc(num_bytes + sizeof(usize) + sizeof(allocator));
  new (data) haste::allocator(move(allocator));
  *((usize*)(data + sizeof(allocator))) = 1;
  return data + sizeof(allocator) + sizeof(usize);
}

inline void delete_buffer(u8* _data, usize offset) {
  auto data = _data - offset - sizeof(usize) - sizeof(allocator);
  haste::allocator allocator = move(*((haste::allocator*)data));
  ((haste::allocator*)data)->~allocator();
  allocator.free(data);
}

trivial_list::trivial_list(trivial_list&& that) {
  switch (that.mode()) {
    case small_opt:
      ::memcpy(this, &that, sizeof(that));
      that._size = 0;
      break;
    case has_alloc:
      new (&_allocator_storage) allocator(move(that.local_allocator()));
      ::memcpy(&this->_data, &that._data, sizeof(that) - sizeof(allocator));
      that.local_allocator().~allocator();
      that._size = 0;
      break;
    case uses_heap:
      ::memcpy(this, &that, sizeof(that));
      ::memset(&that, 0, sizeof(that));
      break;
  }
}

trivial_list::trivial_list(const trivial_list& that) {
  switch (that.mode()) {
    case small_opt:
      ::memcpy(this, &that, sizeof(that));
      break;
    case has_alloc:
      new (&_allocator_storage) allocator(that.local_allocator());
      ::memcpy(&this->_data, &that._data, sizeof(that) - sizeof(allocator));
      break;
    case uses_heap:
      ::memcpy(this, &that, sizeof(that));
      acquire(_data, _offset);
      break;
  }
}

trivial_list::trivial_list(usize item_size, allocator&& allocator)
    : trivial_list(0, item_size, move(allocator)) {}

trivial_list::trivial_list(usize list_size, usize item_size)
    : trivial_list(list_size, item_size, allocator()) {}

trivial_list::trivial_list(usize list_size, usize item_size,
                           allocator&& allocator) {
  auto num_bytes = list_size * item_size;

  if (allocator == haste::allocator() && num_bytes <= max_inplace) {
    set_mode(small_opt);
    set_inplace_size(list_size);
  } else if (allocator != haste::allocator() &&
             num_bytes <= max_inplace - sizeof(allocator)) {
    new (&_allocator_storage) haste::allocator(move(allocator));
    set_mode(has_alloc);
    set_inplace_size(list_size);
  } else {
    _data = create_buffer(move(allocator), num_bytes);
    _offset = 0;
    set_mode(uses_heap);
    set_size(list_size);
  }
}

trivial_list::~trivial_list() {
  switch (mode()) {
    case small_opt:
      break;
    case has_alloc:
      local_allocator().~allocator();
      break;
    case uses_heap:
      if (release(_data, _offset)) {
        delete_buffer(_data, _offset);
      }

      break;
  }
}

trivial_list& trivial_list::operator=(trivial_list&&) { return *this; }

trivial_list& trivial_list::operator=(const trivial_list&) { return *this; }

default_list::default_list(usize list_size, usize item_size)
    : default_list(list_size, item_size, allocator()) {}

default_list::default_list(usize list_size, usize item_size,
                           allocator&& allocator) {
  auto num_bytes = list_size * item_size;
  _data = create_buffer(move(allocator), num_bytes);
  _offset = 0;
  _size = list_size;
}

default_list::~default_list() {
  if (_data && release(_data, _offset)) {
    delete_buffer(_data, _offset);
  }
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

  list<list<int>> l8 = {{1, 2}, {}};
  list<list<int>> l9 = {{1, 2}, {3, 4}};
}

unittest("Test size method") {
  auto l0 = list<int>();
  auto l1 = list<list<int>>();

  assert_eq(l0.size(), 0u);
  assert_eq(l1.size(), 0u);

  auto l2 = list<int>({1});
  auto l3 = list<list<int>>({{1}});

  assert_eq(l2.size(), 1u);
  assert_eq(l3.size(), 1u);

  auto l4 = list<int>({1, 2, 3});
  auto l5 = list<list<int>>({{1}, {2}, {3}});

  assert_eq(l4.size(), 3u);
  assert_eq(l5.size(), 3u);

  auto l6 = list<int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto l7 = list<list<int>>({{1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}});

  assert_eq(l6.size(), 9u);
  assert_eq(l7.size(), 9u);
}

unittest("Test special functions of list of primitive type.") {
  auto l0 = list<int>();
  auto l1 = l0;
  auto l2 = move(l0);

  assert_eq(l0.size(), 0u);
  assert_eq(l1.size(), 0u);
  assert_eq(l2.size(), 0u);

  auto l3 = list<int>({1});
  auto l4 = l3;
  auto l5 = move(l3);

  assert_eq(l3.size(), 0u);
  assert_eq(l4.size(), 1u);
  assert_eq(l5.size(), 1u);

  auto l6 = list<int>(explicit_allocator(), {1, 2});
  auto l7 = l6;
  auto l8 = move(l6);

  assert_eq(l6.size(), 0u);
  assert_eq(l7.size(), 2u);
  assert_eq(l8.size(), 2u);

  auto l9 = list<int>(explicit_allocator(), {1, 2, 3, 4, 5, 6, 7, 8});
  auto l10 = l9;
  auto l11 = move(l9);

  assert_eq(l9.size(), 0u);
  assert_eq(l10.size(), 8u);
  assert_eq(l11.size(), 8u);
}
}
