#include <string.h>
#include <haste/string>
#include <haste/unittest>

namespace haste {

constexpr size_t header_size = sizeof(allocator) + sizeof(usize);

inline usize& get_refcounter(const char* data) {
  constexpr size_t extra = sizeof(usize);
  return *(usize*)(data - extra);
}

void acquire(const char* data) {
  ++get_refcounter(data);
}

void release(const char* data) {
  --get_refcounter(data);
}

string::string(allocator memalloc, const string& that) {
  if (that._size) {
    _size = that._size;

    if (memalloc == that.memalloc()) {
      _data = that._data;
      acquire(_data);
    }
    else {
      _alloc(memalloc);
      ::memcpy(_data, that._data, _size + 1);
    }
  }
}

string::~string() {
  if (_size) {
    if (get_refcounter(_data) == 0) {
      allocator memalloc = this->memalloc();
      auto data = (char*)_data - header_size;
      memalloc.free(_size + header_size + 1, data);
    }
    else {
      release(_data);
    }
  }
}

string::string(usize size, const char* data) {
  _size = size;
  if (_size != 0) {
    _alloc();
    ::memcpy(_data, data, _size + 1);
  }
  else {
    _data = nullptr;
  }
}

allocator string::memalloc() const {
  auto data = (char*)_data - header_size;
  return *(allocator*)data;
}

/* string string::join() {
  return string();
}*/

bool string::_eq(const string& that) const {
  if (_size == that._size) {
    return ::memcmp(_data, that._data, _size) == 0;
  }
  else {
    return false;
  }
}

bool string::_eq(const char* that) const {
  if (that == nullptr) {
    return _size == 0;
  }
  else if (_size == 0) {
    return that[0] == 0;
  }
  else {
    return ::strcmp(_data, that) == 0;
  }
}

void string::_alloc() {
  _alloc(allocator());
}

void string::_alloc(allocator alloc) {
  auto data = (char*)alloc.alloc(_size + header_size + 1);
  new (data + 0) allocator(alloc);
  new (data + sizeof(allocator)) usize(0);
  _data = data + header_size;
}

string::string(usize num_items, join_item items[]) {
  _size = 0;

  for (usize i = 0; i < num_items; ++i) {
    _size += items[i].end - items[i].begin;
  }

  if (_size != 0) {
    _alloc();

    auto dst = _data;
    for (usize i = 0; i < num_items; ++i) {
      auto size = items[i].end - items[i].begin;
      ::memcpy(dst, items[i].begin, size + 1);
      dst += size;
    }
  }
  else {
    _data = nullptr;
  }
}

unittest() {
  auto S = string();
  assert_eq(0u, S.size());
}

unittest() {
  auto S = string("Hello world!");
  assert_eq(S.size(), 12u);
  assert_eq(S.data()[0], 'H');
}

unittest() {
  auto S1 = string("Hello world!");
  auto S2 = S1;
  assert_eq(S1.data(), S2.data());
}

unittest() {
  auto S1 = string("Hello world!");
  auto S2 = string("Hello world!");
  assert_eq(S1, S2);
}

unittest() {
  auto S1 = string("Hello world!");
  auto S2 = string("Hello world?");
  assert_ne(S1, S2);
}

unittest() {
  auto S1 = string("Hello world!");
  auto S2 = S1;
  assert_eq(S1, S2);
}

unittest() {
  auto S1 = string();
  assert_eq(S1, "");
  assert_eq("", S1);
}

unittest() {
  auto S1 = string();
  assert_eq(S1, nullptr);
  assert_eq(nullptr, S1);
}

unittest() {
  auto S1 = string('a');
  assert_ne(S1, nullptr);
  assert_ne(nullptr, S1);
}

unittest() {
  auto S = string::join();
  assert_eq("", S);
}

unittest() {
  auto S = string::join("");
  assert_eq("", S);
}

unittest() {
  auto S = string::join(string());
  assert_eq("", S);
}

unittest() {
  auto S = string();
  assert_ne("Hello world!", S);
}

unittest() {
  auto S = string::join("", string("Hello world!"));
  assert_eq("Hello world!", S);
}

unittest() {
  auto S = string::join(string("Hello world!"), "");
  assert_eq("Hello world!", S);
}

unittest() {
  auto S = string::join("Hello", " ", "world!");
  assert_eq("Hello world!", S);
}

}
