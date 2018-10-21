#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <haste/test>
#include <new>

namespace haste {
namespace detail {}

using ulong_t = unsigned long long;

struct test_t {
  void (*callback)();
  int line;
  const char* file;
};

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

int register_test(void (*callback)(), int line, const char* file, const char*) {
  instance().enlist(callback, line, file);
  return -1;
}
}

trigger_t::~trigger_t() { delete[] _tests; }

trigger_t& trigger_t::enlist(void (*callback)(), int line, const char* file) {
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

  _tests[_size].callback = callback;
  _tests[_size].line = line;
  _tests[_size].file = file;

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

    try {
      trigger._tests[i].callback();
    }
    catch (const std::exception& exception) {
      std::printf(
        "%s:%d: uncaught std::exception; what: %s\n",
        trigger._tests[i].file,
        trigger._tests[i].line,
        exception.what());

      std::fflush(stdout);

      context_instance()->assert_failed();
    }
    catch (...) {
      std::printf(
        "%s:%d: uncaught exception\n",
        trigger._tests[i].file,
        trigger._tests[i].line);

      std::fflush(stdout);

      context_instance()->assert_failed();
    }

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

void assert_almost_eq(const std::string& a, const std::string& b, call_site_t site) {
  auto a_itr = a.begin();
  auto a_end = a.end();
  auto b_itr = b.begin();
  auto b_end = b.end();

  do {
    while (a_itr != a_end && std::isspace(*a_itr)) {
      ++a_itr;
    }

    while (b_itr != b_end && std::isspace(*b_itr)) {
      ++b_itr;
    }
  } while(a_itr != a_end && b_itr != b_end && *a_itr++ == *b_itr++);

  assert_true(a_itr == a_end && b_itr == b_end, site);
}

namespace detail {

void assert_throws(void (*callback)(void*), void* closure, call_site_t site) {
  bool throws = false;

  try
  {
    callback(closure);
  }
  catch (...)
  {
    throws = true;
  }

  assert_true(throws, site);
}

}
}
