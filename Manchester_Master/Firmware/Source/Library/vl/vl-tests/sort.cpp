#include "pch.h"
#include "..\vl\sort.h"

using namespace vl;

TEST(sort, heapsort_debug) {
  int arr[] = { 1, 5, 8, 9, 6, 7, 3, 4, 2, 0 };
  int n = sizeof(arr) / sizeof(arr[0]);
  
  HeapSort(n, arr);

  for (int i = 0; i < n - 1; i++) {
	  EXPECT_TRUE(arr[i+1]>= arr[i]);
  }
}