#include <haste/primitive>

namespace haste {

static_assert(sizeof(i8) == 1, "");
static_assert(sizeof(i16) == 2, "");
static_assert(sizeof(i32) == 4, "");
static_assert(sizeof(i64) == 8, "");

static_assert(sizeof(u8) == 1, "");
static_assert(sizeof(u16) == 2, "");
static_assert(sizeof(u32) == 4, "");
static_assert(sizeof(u64) == 8, "");

static_assert(sizeof(f32) == 4, "");
static_assert(sizeof(f64) == 8, "");
}