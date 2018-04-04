#include <haste/string_view>

#include <cstring>
#include <haste/unittest>

#include <cstdio>

namespace haste {




template <class... T, class R>
	void format(R r, T... t) {
		fwrite("Hello, world!", 12, 1, stdout);
		r = r;
		auto x = { t... };
		x = x;
}



}
