#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <haste/unittest>
#include <new>

namespace haste {
namespace detail {}

using ulong_t = unsigned long long;

struct context_t {
  ulong_t num_failed = 0;
  bool passed = true;

  void reset() { passed = true; }

  void assert_failed() { passed = false; }
};

struct trigger_t {
  trigger_t() = default;
  trigger_t(trigger_t&&) = delete;
  ~trigger_t();

  trigger_t& enlist(void (*)(), int, const char*);

  using test_t = void (*)();
  test_t* _tests = nullptr;
  ulong_t _capacity = 0;
  ulong_t _size = 0;

  static thread_local context_t* context;
};

thread_local context_t* trigger_t::context = nullptr;

namespace {
trigger_t& instance() {
  static trigger_t instance;
  return instance;
}

context_t* context_instance() { return instance().context; }
}

namespace detail {

int register_test(void (*test)(), int line, const char* file, const char*) {
  instance().enlist(test, line, file);
  return -1;
}
}

trigger_t::~trigger_t() { delete[] _tests; }

trigger_t& trigger_t::enlist(void (*test)(), int, const char*) {
  if (_size == _capacity) {
    auto capacity = _capacity * 2;
    capacity = capacity < 16 ? 16 : capacity;

    auto tests = new test_t[capacity];

    for (ulong_t i = 0; i < _size; ++i) {
      tests[i] = _tests[i];
    }

    delete[] _tests;

    _tests = tests;
    _capacity = capacity;
  }

  _tests[_size] = test;
  ++_size;

  return *this;
}

bool run_all_tests() {
  auto& trigger = instance();

  std::printf("Running %llu unittest...\n", trigger._size);

  context_t context;
  trigger.context = &context;

  for (ulong_t i = 0; i < trigger._size; ++i) {
    context.reset();
    trigger._tests[i]();
    if (!context.passed) {
      ++context.num_failed;
    }
  }

  if (context.num_failed) {
    printf("%llu tests failed.\n", context.num_failed);
  } else {
    printf("All tests succeeded.\n");
  }

  return context.num_failed == 0;
}

void assert_true(bool x, call_site_t site) {
  if (!x) {
    std::printf("%s:%d: assertion failed\n", site.file, site.line);
    std::fflush(stdout);

    context_instance()->assert_failed();
  }
}

void assert_false(bool x, call_site_t site) {
  if (x) {
    std::printf("%s:%d: assertion failed\n", site.file, site.line);
    std::fflush(stdout);

    context_instance()->assert_failed();
  }
}

static bool files_eq(const char* a, const char* b) {
  FILE* file_a = fopen(a, "rb");
  FILE* file_b = fopen(b, "rb");

  file_a = file_b;
  file_b = file_a;
  return false;
}

void assert_files_eq(const char* a, const char* b, call_site_t site) {
  assert_true(files_eq(a, b), site);
}

void assert_files_ne(const char* a, const char* b, call_site_t site) {
  assert_false(files_eq(a, b), site);
}


unsigned ulp_dist(float a, float b) {
  union conv_t {
    float f;
    int i;
    static_assert(sizeof(int) == sizeof(float), "");
  };

  conv_t ca, cb;
  ca.f = a;
  cb.f = b;

  if ((ca.i < 0) != (cb.i < 0)) {
    return unsigned(std::abs(ca.i & ~(1 << 31))) +
           unsigned(std::abs(cb.i & ~(1 << 31)));
  } else {
    return std::abs(ca.i - cb.i);
  }
}

bool almost_eq(float a, float b) {
  return std::fabs(a - b) < FLT_EPSILON || ulp_dist(a, b) < 64;
}

unittest() {
  assert_true(ulp_dist(0.0f, -0.0f) == 0);
  assert_true(ulp_dist(1.0000001f, 1.0000002f) != 0);
  assert_false(almost_eq(1.0f, -1.0f));
  assert_true(almost_eq(1.0f, 1.0f));
}

void print_vector(float* v, int s) {
  if (s == 1) {
    std::printf("%f", v[0]);
  } else {
    std::printf("[%f", v[0]);

    for (int i = 1; i < s; ++i) {
      std::printf(", %f", v[i]);
    }

    std::printf("]");
  }
}

namespace detail {

void assert_almost_eq(void* a, void* b, int size, int type, call_site_t site) {
  if (type == 0) {
    auto fa = reinterpret_cast<float*>(a);
    auto fb = reinterpret_cast<float*>(b);

    for (int i = 0; i < size; ++i) {
      if (!almost_eq(fa[i], fb[i])) {
        std::printf("%s:%u: assert_almost_eq(", site.file, site.line);

        print_vector(fa, size);
        std::printf(", ");
        print_vector(fb, size);
        std::printf(") failed\n");

        context_instance()->assert_failed();

        return;
      }
    }
  } else if (type == 1) {
  } else {
    // not gonna happen
  }
}
}
}
