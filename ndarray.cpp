#include <haste/ndarray>
#include <haste/unittest>
#include <cstring>

namespace haste {

bool operator==(const ilist<int>& a, const ndshape& b) {
  if (a.size() != b.size()) {
    return false;
  }
  else {
    for (usize i = 0; i < a.size(); ++i) {
      if (a[i] < 0) {
        return false;
      }
      if (usize(a[i]) != b[i]) {
        return false;
      }
    }

    return true;
  }
}

bool operator!=(const ilist<int>& a, const ndshape& b) {
  return !(a == b);
}

static usize itemsize(ndtype dtype) {
  switch(dtype) {
    case ndtype::i8: return sizeof(i8);
    case ndtype::i16: return sizeof(i16);
    case ndtype::i32: return sizeof(i32);
    case ndtype::i64: return sizeof(i64);
    case ndtype::u8: return sizeof(u8);
    case ndtype::u16: return sizeof(u16);
    case ndtype::u32: return sizeof(u32);
    case ndtype::u64: return sizeof(u64);
    case ndtype::f32: return sizeof(f32);
    case ndtype::f64: return sizeof(f64);
    default: return 0;
  }
}

struct array_impl {
  ndtype dtype;
  ndshape shape;
  ndstride strides;
  u8* data = nullptr;
  allocator memalloc;

  ~array_impl() {
    if (data) {
      memalloc.free(0, data);
    }
  }
};

inline const array_impl* q(const char* impl) {
  return (array_impl*)impl;
}

inline array_impl* q(char* impl) {
  return (array_impl*)impl;
}

ndarray::ndarray() {
  static_assert(sizeof(array_impl) <= sizeof(_impl));
  new (_impl) array_impl();
}

ndarray::ndarray(ndarray&& that) {
  new (_impl) array_impl(std::move(*q(that._impl)));
}

ndarray::ndarray(const ndarray& that) {
  new (_impl) array_impl(*q(that._impl));
}

ndarray::~ndarray() {
  auto _this = q(_impl);
  _this->~array_impl();
}

const ndshape& ndarray::shape() const {
  return q(_impl)->shape;
}

const ndstride& ndarray::strides() const {
  return q(_impl)->strides;
}

ndtype ndarray::dtype() const {
  return q(_impl)->dtype;
}

usize ndarray::ndim() const {
  return q(_impl)->strides.size();
}

usize ndarray::size() const {
  return nbytes() / itemsize();
}

usize ndarray::itemsize() const {
  return haste::itemsize(q(_impl)->dtype);
}

usize ndarray::nbytes() const {
  return shape().size() != 0
    ? foldl([](usize a, usize b) { return a * b; }, itemsize(), shape())
    : 0;
}

void* ndarray::data() {
  return q(_impl)->data;
}

const void* ndarray::data() const {
  return q(_impl)->data;
}

ndarray ndarray::empty(ndshape shape, ndtype dtype) {
  return empty(allocator(), shape, dtype);
}

ndarray ndarray::empty(allocator memalloc, ndshape shape, ndtype dtype) {
  ndarray result;
  auto _this = q(result._impl);

  auto item_size = haste::itemsize(dtype);

  if (shape.size() != 0) {
    _this->strides = scanr([](usize a, usize b) { return a * b; }, item_size, tail(shape));
  }

  _this->shape = shape;
  _this->dtype = dtype;

  size_t nbytes = foldl([](usize a, usize b) { return a * b; }, item_size, shape);

  _this->data = (u8*)memalloc.alloc(nbytes);
  _this->memalloc = memalloc;

  return result;
}

ndarray ndarray::zeros(ndshape shape, ndtype dtype) {
  auto result = empty(shape, dtype);

  if (result.shape().size() != 0) {
    ::memset(result.data(), 0, result.nbytes());
  }

  return result;
}

unittest() {
  auto N = ndarray::empty(ndshape(), ndtype::i32);
  assert_eq(0u, N.nbytes());
}

unittest() {
  auto N = ndarray::empty(ndshape(1), ndtype::i32);
  assert_eq(ndshape(1), N.shape());
  assert_eq(ndtype::i32, N.dtype());
}

unittest() {
  auto N = ndarray::zeros(ndshape(1), ndtype::i32);
  auto V = N.view<u8>();
  assert_eq(ndshape(1), V.shape());
}

unittest() {
  auto strides = ilist<usize>(usize(1));
  assert_eq(8u, ndoffset(strides.data(), 8));
}

unittest() {
  auto strides = ilist<usize>(usize(4), usize(1));
  assert_eq(11u, ndoffset(strides.data(), 2, 3));
}

unittest() {
  auto N = ndarray::zeros(ndshape(1), ndtype::i32);
  auto V = N.view<u8>();
  assert_eq(0, V.at(0));
}

unittest() {
  auto array1 = ndarray::empty(ndshape(1), ndtype::i32);
  assert_eq(4u, array1.nbytes());

  auto array2 = ndarray::empty(ndshape(1, 1), ndtype::i32);
  assert_eq(4u, array2.nbytes());

  auto array3 = ndarray::empty(ndshape(1, 1, 2), ndtype::i32);
  assert_eq(8u, array3.nbytes());

  auto array4 = ndarray::empty(ndshape(4, 3, 2), ndtype::i32);
  assert_eq(4u * 3u * 2u * 4u, array4.nbytes());
}

}

