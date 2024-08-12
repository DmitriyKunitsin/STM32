#include "pch.h"
#include "..\vl\FindIdxByTable.h"

using namespace vl;

//! “ест с массивом нулевой длинны
TEST(FindIdxByTable, FindIdxByTableUp_Length0) {
	int arr[] = {1};
	int n = sizeof(arr) / sizeof(arr[0]);

	uint16_t idx = FindIdxByTableUp(0, arr, 5);

	EXPECT_EQ(idx,0);
}

//! ћассив еденичной длинны значение снизу
TEST(FindIdxByTable, FindIdxByTableUp_Length1_down) {
	int arr[] = { 1 };
	int n = sizeof(arr) / sizeof(arr[0]);

	uint16_t idx = FindIdxByTableUp(n, arr, 0);

	EXPECT_EQ(idx, 0);
}

//! ћассив еденичной длинны значение снизу
TEST(FindIdxByTable, FindIdxByTableUp_Length1_up) {
	int arr[] = { 1 };
	int n = sizeof(arr) / sizeof(arr[0]);

	uint16_t idx = FindIdxByTableUp(n, arr, 2);

	EXPECT_EQ(idx, 0);
}