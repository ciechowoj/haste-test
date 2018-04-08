#include <haste/unittest>




unittest() {
  static_assert(is_fvec<float>, "");
  static_assert(!is_fvec<void*>, "");
  static_assert(!is_fvec<ivec2>, "");
  static_assert(!is_fvec<ivec3>, "");
  static_assert(!is_fvec<ivec4>, "");
  static_assert(is_fvec<vec4>, "");

  static_assert(has_x<vec2> == 1, "");
  static_assert(has_x<vec3> == 1, "");
  static_assert(has_x<ivec2> == 1, "");
  static_assert(!has_x<int>, "");

  static_assert(vec_size<float> == 1, "");
  static_assert(vec_size<int> == 1, "");
  static_assert(vec_size<vec2> == 2, "");
  static_assert(vec_size<vec3> == 3, "");
  static_assert(vec_size<vec4> == 4, "");
  static_assert(vec_size<dvec4> == 4, "");
}

unittest() {
  static_assert(index<short, int> == -1, "");
  static_assert(index<int, int> == 0, "");
  static_assert(index<int, float, int> == 1, "");
  static_assert(index<int, float, double, int> == 2, "");
  static_assert(index<int, float, double, int, bool> == 2, "");
}


