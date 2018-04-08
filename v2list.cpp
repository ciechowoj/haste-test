#include <haste/v2list>
#include <haste/unittest>

namespace haste::v2 {
namespace detail {

inline allocator* get_allocator(const void* data) {
  constexpr size_t extra = sizeof(usize) + sizeof(allocator);
  return (allocator*)((char*)data - extra);
}

inline allocator* get_allocator(const list& list) {
  return get_allocator(list._data);
}

inline usize* get_refcounter(const void* data) {
  constexpr size_t extra = sizeof(usize);
  return (usize*)((char*)data - extra);
}

inline usize* get_refcounter(const list* list) {
  return get_refcounter(list->_data);
}

list::~list() {
  constexpr size_t extra = sizeof(usize) + sizeof(allocator);

  if (_data) {
    auto refcounter = get_refcounter(this);
    if (*refcounter) {
      --*refcounter;
    }
    else {
      allocator* alloc_ptr = (allocator*)((char*)_data - extra);
      allocator alloc = std::move(*alloc_ptr);
      alloc_ptr->~allocator();
      alloc.free(0, alloc_ptr);
    }
  }
}

void list::_alloc(usize item_size) {
  _alloc(explicit_allocator(), item_size);
}

void list::_alloc(allocator alloc, usize item_size) {
  if (_size != 0) {
    constexpr size_t extra = sizeof(usize) + sizeof(allocator);
    char* data = (char*)alloc.alloc(item_size * _size + extra);

    new (data + 0) allocator(alloc);
    new (data + sizeof(allocator)) usize(0);

    _data = data + extra;
  }
}

void list::_copy(usize item_size, const list& that, void (*lambda)(void*, const void*, usize)) {
  if (that._size != 0) {
    _size = that._size;
    _alloc(item_size * _size);
    lambda(_data, that._data, that._size);
  }
  else {
    *this = list();
  }
}

void list::_icopy(usize item_size, const list& that, void (*lambda)(void*, const void*, usize)) {
  if (that._size != 0) {
    auto that_allocator = get_allocator(that);
    if (that_allocator->internal()) {
      _size = that._size;
      _alloc(item_size * _size);
      lambda(_data, that._data, that._size);
    }
    else {
      _data = that._data;
      _size = that._size;
      ++*get_refcounter(this);
    }
  }
  else {
    *this = list();
  }
}

void list::_icopy(usize item_size, usize size, const void* data, void (*lambda)(void*, const void*, usize)) {
  if (size != 0) {
    _size = size;
    _alloc(item_size);
    lambda(_data, data, size);
  }
  else {
    *this = list();
  }
}

}

unittest() {
  auto L = ilist<int>();
  assert_eq(0u, L.size());
}

unittest() {
  auto L = ilist<int>(0);
  assert_eq(1u, L.size());
}

unittest() {
  auto L1 = ilist(42);
  auto L2 = ilist(42, 43, 44);
}

unittest() {
  auto L = ilist<int>(0, 42);
  assert_eq(2u, L.size());
}

unittest() {
  auto L = ilist<int>(0, 42, 314);
  assert_eq(3u, L.size());
}

unittest() {
  auto L = ilist<int>();
  auto M = L;
  assert_eq(0u, M.size());
}

unittest() {
  auto L = ilist<int>(0);
  auto M = L;
  assert_eq(1u, M.size());
}

unittest() {
  auto L = ilist<int>(0);
  auto M = std::move(L);
  assert_eq(1u, M.size());
}

unittest() {
  auto L = ilist<int>(42);
  auto M = L;
  assert_eq(42, M[0]);
}

unittest() {
  auto L = ilist<int>(42);
  auto M = ilist<int>();
  M = L;
  assert_eq(42, M[0]);
}

unittest() {
  auto L = ilist<int>(42, 314);
  auto M = ilist<int>(314);
  M = L;
  assert_eq(42, M[0]);
}

unittest() {
  auto L = ilist<int>(42, 314);
  auto M = ilist<int>(314);
  M = std::move(L);
  assert_eq(42, M[0]);
}

unittest() {
  auto L = ilist<int>(42);
  auto M = L;
  assert_eq(L.data(), M.data());
}

unittest() {
  int L[] = { 42, 43 };
  auto M = ilist<int>(usize(2), (const int*)L);
  assert_eq(2u, M.size());
}

unittest() {
  int L[] = { 42, 43 };
  auto M = ilist<int>(usize(2), (const int*)L);
  assert_eq(2u, M.size());
  assert_eq(42, M[0]);
  assert_eq(43, M[1]);
}

unittest() {
  auto L = list<int>(42);
  auto M = L;
  assert_ne(L.data(), M.data());
}

unittest() {
  auto L = list<int>(42);
  auto M = L.ilist();
  assert_ne(L.data(), M.data());
}

unittest() {
  auto L = list<int>(42);
  auto M = L.ilist();
  assert_eq(42, M[0]);
}

unittest() {
  auto L = ilist<int>();
  auto M = scanr([] (int x, int y) { return x + y; }, 42, ilist_view<int>(L));
  assert_eq(1u, M.size());
}

unittest() {
  auto L = ilist<int>(3);
  auto M = scanr([] (int x, int y) { return x + y; }, 42, ilist_view<int>(L));
  assert_eq(2u, M.size());
  assert_eq(45, M[0]);
  assert_eq(42, M[1]);
}

unittest() {
  auto L = ilist<usize>(usize(1));
  auto M = ilist<usize>();
  assert_eq(1u, L.size());
  M = scanr([] (usize x, usize y) { return x * y; }, usize(4), tail(L));
  assert_eq(1u, M.size());
  assert_eq(4u, M[0]);
}

unittest() {
  auto L = list<int>(45, 42);
  auto M = L.ilist();
  assert_eq(2u, M.size());
  assert_eq(45, M[0]);
  assert_eq(42, M[1]);
}

unittest() {
  auto L = list<int>(0);
  auto M = std::move(L);
  assert_eq(1u, M.size());
}

}
