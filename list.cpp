#include <string.h>
#include <haste/list>
#include <haste/panic>
#include <haste/unittest>

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

inline void acquire(u8* data) {
  __atomic_add_fetch((usize*)(data - sizeof(usize)), 1, __ATOMIC_SEQ_CST);
}

inline bool release(u8* data) {
  return __atomic_sub_fetch((usize*)(data - sizeof(usize)), 1,
                            __ATOMIC_SEQ_CST) == 0;
}

inline usize refcount(u8* data) {
  usize result = 0;
  __atomic_load((usize*)(data - sizeof(usize)), &result, __ATOMIC_SEQ_CST);
  return result;
}

inline allocator& external_allocator(u8* data) {
  return *((allocator*)(data - sizeof(usize) - sizeof(allocator)));
}

inline u8* create_buffer(allocator&& allocator, usize num_bytes) {
  auto data =
      (u8*)allocator.alloc(num_bytes + sizeof(usize) + sizeof(allocator));
  new (data) haste::allocator(move(allocator));
  *((usize*)(data + sizeof(allocator))) = 1;
  return data + sizeof(allocator) + sizeof(usize);
}

inline void delete_buffer(u8* _data) {
  auto data = _data - sizeof(usize) - sizeof(allocator);
  haste::allocator allocator = move(*((haste::allocator*)data));
  ((haste::allocator*)data)->~allocator();
  allocator.free(data);
}

static constexpr usize mode_offset = sizeof(usize) * 8 - 2;
static constexpr usize mode_mask = ~(usize(3u) << mode_offset);
static constexpr usize inplace_offset = (sizeof(usize) - 1) * 8;
static constexpr usize inplace_mask = (~usize(0u) << 8 >> 8) | ~mode_mask;
static constexpr usize max_inplace = sizeof(usize) * 4 - 1;

enum mode_t : u8 {
  small_opt = 0,
  has_alloc = 1,
  uses_heap = 2,
};

inline allocator& local_allocator(trivial_list* _this) {
  return (allocator&)_this->_allocator_storage;
}

inline const allocator& local_allocator(const trivial_list* _this) {
  return (const allocator&)_this->_allocator_storage;
}

inline usize inplace_size(const trivial_list* _this) {
  return (_this->_size & mode_mask) >> inplace_offset;
}

inline void set_size(trivial_list* _this, usize size) {
  _this->_size = (_this->_size & ~mode_mask) | size;
}

inline void set_inplace_size(trivial_list* _this, usize size) {
  _this->_size = (_this->_size & inplace_mask) | (size << inplace_offset);
}

inline mode_t mode(const trivial_list* _this) {
  return mode_t(_this->_size >> mode_offset);
}

inline void set_mode(trivial_list* _this, mode_t mode) {
  _this->_size = (_this->_size & mode_mask) | usize(mode) << mode_offset;
}

trivial_list::trivial_list(trivial_list&& that) {
  switch (mode(&that)) {
    case small_opt:
      ::memcpy(this, &that, sizeof(that));
      that._size = 0;
      break;
    case has_alloc:
      new (&_allocator_storage) haste::allocator(move(local_allocator(&that)));
      ::memcpy(&this->_data, &that._data,
               sizeof(that) - sizeof(haste::allocator));
      local_allocator(&that).~allocator();
      that._size = 0;
      break;
    case uses_heap:
      ::memcpy(this, &that, sizeof(that));
      ::memset(&that, 0, sizeof(that));
      break;
  }
}

trivial_list::trivial_list(const trivial_list& that) {
  switch (mode(&that)) {
    case small_opt:
      ::memcpy(this, &that, sizeof(that));
      break;
    case has_alloc:
      new (&_allocator_storage) haste::allocator(local_allocator(&that));
      ::memcpy(&this->_data, &that._data,
               sizeof(that) - sizeof(haste::allocator));
      break;
    case uses_heap:
      ::memcpy(this, &that, sizeof(that));
      acquire(_data);
      break;
  }
}

trivial_list::trivial_list(usize item_size, haste::allocator&& allocator)
    : trivial_list(max_inplace / item_size * item_size, item_size,
                   move(allocator)) {}

trivial_list::trivial_list(usize list_size, usize item_size,
                           haste::allocator&& allocator) {
  auto num_bytes = list_size * item_size;

  if (allocator == haste::allocator() && num_bytes <= max_inplace) {
    set_mode(this, small_opt);
    set_inplace_size(this, list_size);
  } else if (allocator != haste::allocator() &&
             num_bytes <= max_inplace - sizeof(allocator)) {
    new (&_allocator_storage) haste::allocator(move(allocator));
    set_mode(this, has_alloc);
    set_inplace_size(this, list_size);
  } else {
    _data = create_buffer(move(allocator), num_bytes);
    _offset = 0;
    set_mode(this, uses_heap);
    set_size(this, list_size);
  }
}

trivial_list::trivial_list(const void* data, usize list_size, usize item_size,
                           haste::allocator&& allocator)
    : trivial_list(list_size, item_size, move(allocator)) {
  ::memcpy(this->data(), data, list_size * item_size);
}

trivial_list::~trivial_list() {
  switch (mode(this)) {
    case small_opt:
      break;
    case has_alloc:
      local_allocator(this).~allocator();
      break;
    case uses_heap:
      if (release(_data)) {
        delete_buffer(_data);
      }

      break;
  }
}

trivial_list& trivial_list::operator=(trivial_list&& that) {
  this->~trivial_list();
  new (this) trivial_list(that);
  return *this;
}

trivial_list& trivial_list::operator=(const trivial_list& that) {
  trivial_list temp = that;
  *this = move(temp);
  return *this;
}

void trivial_list::init(const void* data, usize size) {
  ::memcpy(this->data(), data, size);
}

usize trivial_list::expand(usize capacity, usize item_size) {
  capacity = max(capacity * 3 / 2, max_inplace / item_size, 1);
  trivial_list temp(capacity, item_size, haste::allocator(this->allocator()));
  ::memcpy(temp.data(), this->data(), this->size() * item_size);
  swap(temp);
  return capacity;
}

void trivial_list::shrink(usize list_size, usize item_size) {
  if (this->size() != list_size) {
    trivial_list temp(list_size, item_size,
                      haste::allocator(this->allocator()));
    ::memcpy(temp.data(), this->data(), list_size * item_size);
    swap(temp);
  }
}

void trivial_list::swap(trivial_list& that) {
  alignas(alignof(trivial_list)) char temp[sizeof(trivial_list)];
  ::memcpy(temp, this, sizeof(trivial_list));
  ::memcpy(this, &that, sizeof(trivial_list));
  ::memcpy(&that, temp, sizeof(trivial_list));
}

void trivial_list::view(void* result, usize item_size) const {
  switch (mode(this)) {
    case small_opt:
      ((const void**)result)[0] = this;
      ((const void**)result)[1] = (u8*)this + inplace_size(this) * item_size;
      break;
    case has_alloc:
      ((const void**)result)[0] = &_data;
      ((const void**)result)[1] = (u8*)&_data + inplace_size(this) * item_size;
      break;
    default:
      ((const void**)result)[0] = _data;
      ((const void**)result)[1] = (u8*)_data + (_size & mode_mask) * item_size;
      break;
  }
}

usize trivial_list::size() const {
  return mode(this) == uses_heap ? _size & mode_mask : inplace_size(this);
}

void* trivial_list::data() {
  switch (mode(this)) {
    case small_opt:
      return this;
    case has_alloc:
      return &_data;
    default:
      return _data;
  }
}

trivial_list trivial_list::steal(usize item_size) {
  switch (mode(this)) {
    case small_opt:
    case has_alloc:
      return move(*this);
    default:
      if (refcount(_data) == 1) {
        return move(*this);
      } else {
        return clone(item_size);
      }
  }
}

trivial_list trivial_list::clone(usize item_size) const {
  switch (mode(this)) {
    case small_opt:
    case has_alloc:
      return *this;
    default: {
      auto list_size = size();
      trivial_list result(list_size, item_size,
                          haste::allocator(external_allocator(_data)));
      ::memcpy(result._data, _data, list_size * item_size);
      return result;
    }
  }
}

trivial_list trivial_list::slice(usize begin, usize end,
                                 usize item_size) const {
  panic_if(begin <= end && end <= size(), panic_reason::index_error);

  switch (mode(this)) {
    case small_opt: {
      trivial_list result(end - begin, item_size, haste::allocator());
      ::memcpy(&result, this, inplace_size(this) * item_size);
      return result;
    }

    case has_alloc: {
      trivial_list result(end - begin, item_size,
                          haste::allocator(local_allocator(this)));
      ::memcpy(&result._data, &_data, inplace_size(this) * item_size);
      return result;
    }

    default: {
      trivial_list result;
      ::memcpy(&result, this, sizeof(result));
      acquire(_data);
      result._offset += begin;
      result._size = end - begin;
      return result;
    }
  }
}

const void* trivial_list::data() const { return ((trivial_list*)this)->data(); }

haste::allocator trivial_list::allocator() const {
  switch (mode(this)) {
    case small_opt:
      return haste::allocator();

    case has_alloc:
      return local_allocator(this);

    default:
      return external_allocator(_data);
  }
}

default_list::default_list(usize list_size, usize item_size,
                           allocator&& allocator) {
  auto num_bytes = list_size * item_size;
  _data = create_buffer(move(allocator), num_bytes);
  _offset = 0;
  _size = list_size;
}

default_list::~default_list() {
  if (_data && release(_data)) {
    delete_buffer(_data);
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
  auto l1 = list<int>({1}, allocator());
  auto l2 = list<int>({1, 2}, allocator());
  auto l3 = list<int>({1, 2, 3}, allocator());
  auto l4 = list<int>({1, 2, 3, 4}, allocator());
  auto l5 = list<int>({1, 2, 3, 4, 5}, allocator());
  auto l6 = list<int>({1, 2, 3, 4, 5, 6}, allocator());
  auto l7 = list<int>({1, 2, 3, 4, 5, 6, 7}, allocator());
  auto l8 = list<int>({1, 2, 3, 4, 5, 6, 7, 8}, allocator());
  auto l9 = list<int>({1, 2, 3, 4, 5, 6, 7, 8, 9}, allocator());
}

unittest("Create lists of different lengths with explicit allocator.") {
  auto l0 = list<int>(explicit_allocator());
  auto l1 = list<int>({1}, explicit_allocator());
  auto l2 = list<int>({1, 2}, explicit_allocator());
  auto l3 = list<int>({1, 2, 3}, explicit_allocator());
  auto l4 = list<int>({1, 2, 3, 4}, explicit_allocator());
  auto l5 = list<int>({1, 2, 3, 4, 5}, explicit_allocator());
  auto l6 = list<int>({1, 2, 3, 4, 5, 6}, explicit_allocator());
  auto l7 = list<int>({1, 2, 3, 4, 5, 6, 7}, explicit_allocator());
  auto l8 = list<int>({1, 2, 3, 4, 5, 6, 7, 8}, explicit_allocator());
  auto l9 = list<int>({1, 2, 3, 4, 5, 6, 7, 8, 9}, explicit_allocator());
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

unittest("Test constructors of list of primitive type.") {
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

  auto l6 = list<int>({1, 2}, explicit_allocator());
  auto l7 = l6;
  auto l8 = move(l6);

  assert_eq(l6.size(), 0u);
  assert_eq(l7.size(), 2u);
  assert_eq(l8.size(), 2u);

  auto l9 = list<int>({1, 2, 3, 4, 5, 6, 7, 8}, explicit_allocator());
  auto l10 = l9;
  auto l11 = move(l9);

  assert_eq(l9.size(), 0u);
  assert_eq(l10.size(), 8u);
  assert_eq(l11.size(), 8u);
}

unittest("Test assignment operators of list of primitive type.") {
  auto l0 = list<int>();
  auto l1 = list<int>({1});
  auto l2 = list<int>({1, 2});
  auto l3 = list<int>({1, 2, 3, 4});
  auto l4 = list<int>({1, 2, 3, 4, 5, 6, 7, 8});
  auto l5 = list<int>();

  l5 = l0;
  assert_eq(l5.size(), 0u);

  l5 = l1;
  assert_eq(l5.size(), 1u);

  l5 = l2;
  assert_eq(l5.size(), 2u);

  l5 = l3;
  assert_eq(l5.size(), 4u);

  l5 = l4;
  assert_eq(l5.size(), 8u);

  l5 = list<int>();

  l5 = move(l0);
  assert_eq(l5.size(), 0u);

  l5 = move(l1);
  assert_eq(l5.size(), 1u);

  l5 = move(l2);
  assert_eq(l5.size(), 2u);

  l5 = move(l3);
  assert_eq(l5.size(), 4u);

  l5 = move(l4);
  assert_eq(l5.size(), 8u);
}

unittest() {
  // copying const to const is refcounting (if big enough)
  auto l0 = list<const int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto l1 = list<const int>(l0);
  assert_eq(l0.data(), l1.data());

  // moving const to const is moving
  auto l2 = list<const int>(move(l0));
  assert_eq(l1.data(), l2.data());
  assert_eq(l0.size(), 0u);

  // copying const to non-const is cloning
  auto l3 = list<int>(l1);
  assert_ne(l1.data(), l3.data());

  // moving const to non-const is cloning or moving if the const is singleton
  auto l4 = list<const int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto l5 = list<const int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto l6 = l5;

  auto l7 = list<int>(move(l4));
  auto l8 = list<int>(move(l5));

  assert_eq(l4.size(), 0u);
  assert_eq(l5.size(), 9u);

  // copying non-const to non-const is cloning
  auto l10 = list<int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto l11 = list<int>(l10);

  assert_ne(l10.data(), l11.data());

  // moving non-const to non-const is moving
  auto l12 = list<int>(move(l10));
  assert_ne(l11.data(), l12.data());
  assert_eq(l10.size(), 0u);
  assert_eq(l12.size(), 9u);

  // copying non-const to const is cloning
  auto l13 = list<const int>(l11);
  assert_ne(l11.data(), l13.data());
  assert_eq(l11.size(), 9u);
  assert_eq(l13.size(), 9u);

  // moving non-const to const is moving
  auto l14 = list<int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto l15 = list<const int>(move(l14));

  assert_eq(l14.size(), 0u);
  assert_eq(l15.size(), 9u);
}

unittest() {
  auto l0 = list<const int>();

  assert_eq(l0.size(), 0u);
  auto l1 = l0.slice(0, 0);

  assert_eq(l1.size(), 0u);

  auto l2 = list<const int>({1});

  assert_eq(l2.size(), 1u);

  auto l3 = l2.slice(0, 1);

  assert_eq(l3.size(), 1u);

  auto l4 = l2.slice(0, 0);

  assert_eq(l4.size(), 0u);

  auto l5 = l2.slice(1, 1);

  assert_eq(l5.size(), 0u);
}
}
