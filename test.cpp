#include <haste/unittest>

int main() {
	return haste::run_all_tests() ? 0 : 1;
}

unittest() {
	haste::assert_true(true);
}
