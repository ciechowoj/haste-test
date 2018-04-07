#include <cstdlib>
#include <cstring>
#include <haste/allocator>
#include <haste/panic>
#include <haste/unittest>

namespace haste {

struct allocator::_impl_t {
  void* (*alloc)(_impl_t*, usize) = [](_impl_t* impl, usize size) -> void* {
    usize total = size + sizeof(usize);
    void* memory = ::malloc(total);

    if (memory) {
      __atomic_fetch_add(&impl->usage, total, __ATOMIC_SEQ_CST);
      *((usize*)memory) = total;
      return (char*)memory + sizeof(usize);
    } else {
      return nullptr;
    }
  };

  void* (*realloc)(_impl_t*, usize, void*) = [](_impl_t* impl, usize size,
                                                void* memory) -> void* {
    if (memory) {
      usize total = *((usize*)memory - 1);
      usize usage = __atomic_fetch_sub(&impl->usage, total, __ATOMIC_SEQ_CST);

      if (usage < total) {
        panic("heap corruption");
      }

      total = size + sizeof(usize);
      memory = ::realloc((char*)memory - sizeof(usize), total);

      if (memory) {
        __atomic_fetch_add(&impl->usage, total, __ATOMIC_SEQ_CST);
        *((usize*)memory) = total;
        return (char*)memory + sizeof(usize);
      } else {
        return nullptr;
      }
    } else {
      return impl->alloc(impl, size);
    }
  };

  void (*free)(_impl_t*, void*) = [](_impl_t* impl, void* memory) {
    if (memory) {
      usize total = *((usize*)memory - 1);
      usize usage = __atomic_fetch_sub(&impl->usage, total, __ATOMIC_SEQ_CST);

      if (usage < total) {
        panic("heap corruption");
      }

      ::free((char*)memory - sizeof(usize));
    }
  };

  void (*del)(_impl_t*) = [](_impl_t*) {};

  usize refcount = 0;
  usize capacity = 0;
  usize usage = 0;
  const bool internal = false;

  void acquire() { __atomic_add_fetch(&refcount, 1, __ATOMIC_SEQ_CST); }
  bool release() {
    return __atomic_sub_fetch(&refcount, 1, __ATOMIC_SEQ_CST) == 0;
  }

  ~_impl_t() {
    if (refcount != 0) {
      panic("allocator reference count corruption");
    }

    if (usage != 0) {
      warn("memory leak");
    }
  }
};

allocator::_impl_t allocator::_default;

allocator::allocator(const allocator& that) : _impl(that._impl) {
  if (_impl) {
    _impl->acquire();
  }
}

allocator::allocator(allocator&& that) : _impl(that._impl) {
  that._impl = nullptr;
}

allocator::~allocator() {
  if (_impl && _impl->release()) {
    _impl->del(_impl);
  }
}

allocator& allocator::operator=(const allocator& that) {
  if (_impl && _impl->release()) {
    _impl->del(_impl);
  }

  _impl = that._impl;

  if (_impl) {
    _impl->acquire();
  }

  return *this;
}

allocator& allocator::operator=(allocator&& that) {
  if (_impl && _impl->release()) {
    _impl->del(_impl);
  }

  _impl = that._impl;
  that._impl = nullptr;

  return *this;
}

void* allocator::alloc(usize size) const {
  auto impl = _impl ? _impl : &_default;
  return impl->alloc(impl, size);
}

void* allocator::realloc(usize size, void* memory) const {
  auto impl = _impl ? _impl : &_default;
  return impl->realloc(impl, size, memory);
}

void allocator::free(usize, void* memory) const {
  auto impl = _impl ? _impl : &_default;
  impl->free(impl, memory);
}

bool allocator::internal() const {
  auto impl = _impl ? _impl : &_default;
  return impl->internal;
}

usize allocator::capacity() const {
  auto impl = _impl ? _impl : &_default;
  return impl->capacity;
}

usize allocator::usage() const {
  auto impl = _impl ? _impl : &_default;
  return impl->usage;
}

allocator::allocator(_impl_t* impl) : _impl(impl) {
  if (_impl) {
    _impl->acquire();
  }
}

allocator explicit_allocator() {
  return allocator(&allocator::_default);
}

unittest("Allocator object should have size of the pointer.") {
  static_assert(sizeof(allocator) == sizeof(void*), "");
}

unittest("Default allocator shouldn't crash.") {
  allocator allocator1 = allocator();

  void* memory1 = allocator1.alloc(1024);
  allocator1.free(1024, memory1);

  allocator allocator2 = allocator1;
  void* memory2 = allocator2.alloc(128);
  allocator2.free(128, memory2);
}

unittest("Default allocator should be represented as zeros.") {
  allocator default_allocator;

  char zeros[sizeof(default_allocator)] = {0};

  assert_true(::memcmp(&default_allocator, zeros, sizeof(zeros)) == 0);
}

struct allocator_storage {
  alignas(allocator) char _data[sizeof(allocator)] = {0};
};

unittest() {
  allocator a0;
  allocator a1 = move(a0);

  allocator_storage storage;
  (allocator&)storage = move(a1);
}


}
