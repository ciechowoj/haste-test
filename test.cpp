#include <haste/unittest.hpp>

int main() {
	return haste::run_all_tests() ? 0 : 1;
}

unittest() {
	haste::assert_true(true);
}
