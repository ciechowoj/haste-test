#include <cstdlib>
#include <haste/allocator>
#include <haste/panic>
#include <haste/unittest>

namespace haste {

struct allocator::_impl_t {
  void* (*alloc)(_impl_t*, usize) = [](_impl_t*, usize) -> void* {
    return nullptr;
  };

  void* (*realloc)(_impl_t*, usize, void*) =
      [](_impl_t*, usize, void*) -> void* { return nullptr; };

  void (*free)(_impl_t*, void*) = [](_impl_t*, void*) {};

  void (*del)(_impl_t*) = [](_impl_t*) {};

  usize refcount = 0;
  usize capacity = 0;
  usize usage = 0;

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

allocator::_impl_t allocator::_def;

allocator::allocator() : _impl(&_def) { _impl->acquire(); }

allocator::allocator(const allocator& that) : _impl(that._impl) {
  _impl->acquire();
}

allocator::allocator(allocator&& that) : _impl(that._impl) {
  that._impl = &_def;
  _def.acquire();
}

allocator::~allocator() {
  if (_impl->release()) {
    _impl->del(_impl);
  }
}

allocator& allocator::operator=(const allocator& that) {
  if (_impl->release()) {
    _impl->del(_impl);
  }

  _impl = that._impl;
  _impl->acquire();

  return *this;
}

allocator& allocator::operator=(allocator&& that) {
  if (_impl->release()) {
    _impl->del(_impl);
  }

  _impl = that._impl;
  that._impl = &_def;
  _def.acquire();

  return *this;
}

void* allocator::alloc(usize size) { return _impl->alloc(_impl, size); }

void* allocator::realloc(usize size, void* memory) {
  return _impl->realloc(_impl, size, memory);
}

void allocator::free(void* memory) { _impl->free(_impl, memory); }

usize allocator::capacity() const { return _impl->capacity; }

usize allocator::usage() const { return _impl->usage; }

allocator::allocator(_impl_t* impl) : _impl(impl) { _impl->acquire(); }

allocator default_allocator() {
  struct impl_t : public allocator::_impl_t {
    impl_t() {
      alloc = [](_impl_t* impl, usize size) -> void* {
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

      realloc = [](_impl_t* impl, usize size, void* memory) -> void* {
        if (memory) {
          usize total = *((usize*)memory - 1);
          usize usage =
              __atomic_fetch_sub(&impl->usage, total, __ATOMIC_SEQ_CST);

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

      free = [](_impl_t* impl, void* memory) {
        if (memory) {
          usize total = *((usize*)memory - 1);
          usize usage =
              __atomic_fetch_sub(&impl->usage, total, __ATOMIC_SEQ_CST);

          if (usage < total) {
            panic("heap corruption");
          }

          ::free((char*)memory - sizeof(usize));
        }
      };

      capacity = usize_max;
    }
  };

  static impl_t singleton;

  return allocator(&singleton);
}

unittest() { static_assert(sizeof(allocator) == sizeof(void*), ""); }

unittest() {
  allocator allocator1 = default_allocator();

  void* memory1 = allocator1.alloc(1024);
  allocator1.free(memory1);

  allocator allocator2 = allocator1;
  void* memory2 = allocator2.alloc(128);
  allocator2.free(memory2);
}
}
