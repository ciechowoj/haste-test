#include <haste/ndarray>
#include <haste/unittest>

namespace haste::nd {

static usize itemsize(nd::dtype dtype) {
  switch(dtype) {
    case dtype::i8: return sizeof(i8);
    case dtype::i16: return sizeof(i16);
    case dtype::i32: return sizeof(i32);
    case dtype::i64: return sizeof(i64);
    case dtype::u8: return sizeof(u8);
    case dtype::u16: return sizeof(u16);
    case dtype::u32: return sizeof(u32);
    case dtype::u64: return sizeof(u64);
    case dtype::f32: return sizeof(f32);
    case dtype::f64: return sizeof(f64);
    default: return 0;
  }
}

struct array_buffer {
  usize refcount;
  usize nbytes;
  u8* data;
};

struct array_impl {
  nd::dtype dtype;
  ilist<usize> strides;
  u8* data;
  array_buffer* buffer;
};

inline const array_impl* q(const char* impl) {
  return (array_impl*)impl;
}

inline array_impl* q(char* impl) {
  return (array_impl*)impl;
}

array::array() {
  static_assert(sizeof(array_impl) <= sizeof(_impl));
  new (_impl) array_impl();
}

array::~array() {

}

nd::shape array::shape() const {
  return nd::shape();
}

nd::strides array::strides() const {
  return nd::strides();
}

usize array::ndim() const {
  return q(_impl)->strides.size();
}

usize array::size() const {
  return nbytes() / itemsize();
}

usize array::itemsize() const {
  return haste::nd::itemsize(q(_impl)->dtype);
}

usize array::nbytes() const {
  return 0;
}


array zeros();

array empty(nd::shape shape, nd::dtype dtype) {
  dtype = dtype;
  shape = shape;

  return array();
}

unittest() {
  auto array0 = empty(shape(), dtype::i32);

  assert_eq(0u, array0.nbytes());

  auto array1 = empty(shape(1), dtype::i32);

/*  assert_eq(4u, array1.nbytes());

  auto array2 = empty({ 1, 1 }, dtype::i32);

  assert_eq(4u, array2.nbytes());

  auto array3 = empty({ 1, 1, 2 }, dtype::i32);

  assert_eq(8u, array3.nbytes());

  auto array4 = empty({ 4, 3, 2 }, dtype::i32);

  assert_eq(4u * 3u * 2u * 4u, array4.nbytes());*/
}


}
