#include <haste/v2list>
#include <haste/unittest>

namespace haste::v2 {
namespace detail {

inline allocator* get_allocator(const void* data) {
  constexpr size_t extra = sizeof(usize) + sizeof(allocator);
  return (allocator*)((char*)data - extra);
}

inline allocator* get_allocator(const ilist& list) {
  return get_allocator(list._data);
}

inline usize* get_refcounter(const void* data) {
  constexpr size_t extra = sizeof(usize);
  return (usize*)((char*)data - extra);
}

inline usize* get_refcounter(const ilist* list) {
  return get_refcounter(list->_data);
}

ilist::~ilist() {
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

void ilist::_alloc(usize item_size) {
  _alloc(explicit_allocator(), item_size);
}

void ilist::_alloc(allocator alloc, usize item_size) {
  constexpr size_t extra = sizeof(usize) + sizeof(allocator);
  char* data = (char*)alloc.alloc(item_size * _size + extra);

  new (data + 0) allocator(alloc);
  new (data + sizeof(allocator)) usize(0);

  _data = data + extra;
}

void ilist::_copy(usize size, const ilist& that, void (*lambda)(void*, const void*, usize)) {
  if (that._size != 0) {
    auto that_allocator = get_allocator(that);
    if (that_allocator->internal()) {
      _size = that._size;
      _alloc(size * _size);
      lambda(_data, that._data, that._size);
    }
    else {
      _data = that._data;
      _size = that._size;
      ++*get_refcounter(this);
    }
  }
  else {
    *this = ilist();
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

}
