#include "strv.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_trim_should_remove_outer_spaces_and_tabs(void) {
  strv input = strv_from("\t  hello world  \t");
  strv out = strv_trim(input);

  TEST_ASSERT_EQUAL_UINT64(11, out.len);
  TEST_ASSERT_EQUAL_MEMORY("hello world", out.data, out.len);
}

void test_split_should_return_all_parts(void) {
  strv input = strv_from("a,b,c");
  Darr parts = strv_split(input, ',');

  TEST_ASSERT_EQUAL_UINT64(3, parts.size);

  strv* p0 = (strv*)darr_get(&parts, 0);
  strv* p1 = (strv*)darr_get(&parts, 1);
  strv* p2 = (strv*)darr_get(&parts, 2);

  TEST_ASSERT_EQUAL_MEMORY("a", p0->data, p0->len);
  TEST_ASSERT_EQUAL_MEMORY("b", p1->data, p1->len);
  TEST_ASSERT_EQUAL_MEMORY("c", p2->data, p2->len);

  darr_destroy(&parts);
}

void test_find_and_prefix_suffix_should_match_expected_positions(void) {
  strv input = strv_from("abcde");

  TEST_ASSERT_EQUAL_UINT64(1, strv_find_char(input, 'b'));
  TEST_ASSERT_EQUAL_UINT64(2, strv_find(input, strv_from("cd")));
  TEST_ASSERT_TRUE(strv_starts_with(input, strv_from("ab")));
  TEST_ASSERT_TRUE(strv_ends_with(input, strv_from("de")));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_trim_should_remove_outer_spaces_and_tabs);
  RUN_TEST(test_split_should_return_all_parts);
  RUN_TEST(test_find_and_prefix_suffix_should_match_expected_positions);
  return UNITY_END();
}
