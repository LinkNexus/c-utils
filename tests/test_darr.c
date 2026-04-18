#include "darr.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static bool is_even(const void* el, size_t idx) {
  (void)idx;
  return (*(const int*)el % 2) == 0;
}

void test_push_back_and_get_should_store_items(void) {
  Darr arr = darr_create(sizeof(int));

  int a = 10;
  int b = 20;

  TEST_ASSERT_TRUE(darr_push_back(&arr, &a));
  TEST_ASSERT_TRUE(darr_push_back(&arr, &b));

  TEST_ASSERT_EQUAL_UINT64(2, arr.size);
  TEST_ASSERT_EQUAL_INT(10, *(int*)darr_get(&arr, 0));
  TEST_ASSERT_EQUAL_INT(20, *(int*)darr_get(&arr, 1));

  darr_destroy(&arr);
}

void test_insert_at_should_shift_elements_to_the_right(void) {
  Darr arr = darr_create(sizeof(int));

  int v1 = 1;
  int v2 = 3;
  int insert = 2;

  TEST_ASSERT_TRUE(darr_push_back(&arr, &v1));
  TEST_ASSERT_TRUE(darr_push_back(&arr, &v2));
  TEST_ASSERT_TRUE(darr_insert_at(&arr, 1, &insert));

  TEST_ASSERT_EQUAL_UINT64(3, arr.size);

  TEST_ASSERT_EQUAL_INT(1, *(int*)darr_get(&arr, 0));
  TEST_ASSERT_EQUAL_INT(2, *(int*)darr_get(&arr, 1));
  TEST_ASSERT_EQUAL_INT(3, *(int*)darr_get(&arr, 2));

  darr_destroy(&arr);
}

void test_remove_at_should_shift_elements_to_the_left(void) {
  Darr arr = darr_create(sizeof(int));

  int v1 = 1;
  int v2 = 2;
  int v3 = 3;

  TEST_ASSERT_TRUE(darr_push_back(&arr, &v1));
  TEST_ASSERT_TRUE(darr_push_back(&arr, &v2));
  TEST_ASSERT_TRUE(darr_push_back(&arr, &v3));

  TEST_ASSERT_TRUE(darr_remove_at(&arr, 1));
  TEST_ASSERT_EQUAL_UINT64(2, arr.size);
  TEST_ASSERT_EQUAL_INT(3, *(int*)darr_get(&arr, 1));

  darr_destroy(&arr);
}

void test_filter_should_keep_matching_items(void) {
  Darr arr = darr_create(sizeof(int));
  int data[] = {1, 2, 3, 4, 5, 6};

  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    TEST_ASSERT_TRUE(darr_push_back(&arr, &data[i]));
  }

  Darr filtered = darr_filter(&arr, is_even);

  TEST_ASSERT_EQUAL_UINT64(3, filtered.size);
  TEST_ASSERT_EQUAL_INT(2, *(int*)darr_get(&filtered, 0));
  TEST_ASSERT_EQUAL_INT(4, *(int*)darr_get(&filtered, 1));
  TEST_ASSERT_EQUAL_INT(6, *(int*)darr_get(&filtered, 2));

  darr_destroy(&arr);
  darr_destroy(&filtered);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_push_back_and_get_should_store_items);
  RUN_TEST(test_insert_at_should_shift_elements_to_the_right);
  RUN_TEST(test_remove_at_should_shift_elements_to_the_left);
  RUN_TEST(test_filter_should_keep_matching_items);
  return UNITY_END();
}
