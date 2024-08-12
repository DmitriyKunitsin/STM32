#include "pch.h"
#include "..\vl\FindIdxByTable.h"

using namespace vl;

//! ���� � �������� ������� ������
TEST(FindIdxByTable, FindIdxByTableUp_Length0) {
	int arr[] = {1};
	int n = sizeof(arr) / sizeof(arr[0]);

	uint16_t idx = FindIdxByTableUp(0, arr, 5);

	EXPECT_EQ(idx,0);
}

//! ������ ��������� ������ �������� �����
TEST(FindIdxByTable, FindIdxByTableUp_Length1_down) {
	int arr[] = { 1 };
	int n = sizeof(arr) / sizeof(arr[0]);

	uint16_t idx = FindIdxByTableUp(n, arr, 0);

	EXPECT_EQ(idx, 0);
}

//! ������ ��������� ������ �������� �����
TEST(FindIdxByTable, FindIdxByTableUp_Length1_up) {
	int arr[] = { 1 };
	int n = sizeof(arr) / sizeof(arr[0]);

	uint16_t idx = FindIdxByTableUp(n, arr, 2);

	EXPECT_EQ(idx, 0);
}