#include "pch.h"
#include "..\vl\util.h"

using namespace vl;

TEST(util, swap_debug) {
	int a = 1, b = 2;
	swap(a,b);
	EXPECT_EQ(b, 1);
	EXPECT_EQ(a, 2);
}

TEST(util, min_debug) {
	int a = 1, b = 2;
	int c=min(a, b);
	EXPECT_EQ(a, c);
}

TEST(util, max_debug) {
	int a = 1, b = 2;
	int c = max(a, b);
	EXPECT_EQ(b, c);
}