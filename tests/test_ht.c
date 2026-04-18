#include "ht.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_set_and_get_should_store_values_by_key(void) {
  HashTable table = ht_create(sizeof(int));

  int value = 123;
  ht_set(&table, "alpha", &value);

  int* got = (int*)ht_get(&table, "alpha");
  TEST_ASSERT_NOT_NULL(got);
  TEST_ASSERT_EQUAL_INT(123, *got);

  ht_destroy(&table);
}

void test_set_existing_key_should_overwrite_value(void) {
  HashTable table = ht_create(sizeof(int));

  int v1 = 1;
  int v2 = 2;

  ht_set(&table, "k", &v1);
  ht_set(&table, "k", &v2);

  int* got = (int*)ht_get(&table, "k");
  TEST_ASSERT_NOT_NULL(got);
  TEST_ASSERT_EQUAL_INT(2, *got);
  TEST_ASSERT_EQUAL_UINT64(1, table.size);

  ht_destroy(&table);
}

void test_delete_should_remove_key(void) {
  HashTable table = ht_create(sizeof(int));

  int value = 7;
  ht_set(&table, "to-delete", &value);
  TEST_ASSERT_NOT_NULL(ht_get(&table, "to-delete"));

  ht_delete(&table, "to-delete");

  TEST_ASSERT_NULL(ht_get(&table, "to-delete"));
  TEST_ASSERT_EQUAL_UINT64(0, table.size);

  ht_destroy(&table);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_set_and_get_should_store_values_by_key);
  RUN_TEST(test_set_existing_key_should_overwrite_value);
  RUN_TEST(test_delete_should_remove_key);
  return UNITY_END();
}
