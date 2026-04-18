#include "dstr.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_create_and_append_should_build_string(void) {
  dstr s = dstr_create();

  dstr_append(&s, "hello");
  dstr_append_char(&s, ' ');
  dstr_append(&s, "world");

  TEST_ASSERT_EQUAL_STRING("hello world", s);
  TEST_ASSERT_EQUAL_UINT64(11, dstr_len(s));

  dstr_destroy(s);
}

void test_concat_should_return_new_combined_string(void) {
  dstr left = dstr_from("foo");
  dstr right = dstr_from("bar");

  dstr joined = dstr_concat(left, right);

  TEST_ASSERT_EQUAL_STRING("foobar", joined);
  TEST_ASSERT_EQUAL_STRING("foo", left);
  TEST_ASSERT_EQUAL_STRING("bar", right);

  dstr_destroy(left);
  dstr_destroy(right);
  dstr_destroy(joined);
}

void test_format_functions_should_write_expected_text(void) {
  dstr base = dstr_fmt("num=%d", 42);

  TEST_ASSERT_EQUAL_STRING("num=42", base);

  dstr_append_fmt(&base, " and %s", "ok");
  TEST_ASSERT_EQUAL_STRING("num=42 and ok", base);

  dstr tail = dstr_concat_fmt(base, " (%0.1f)", 3.5);
  TEST_ASSERT_EQUAL_STRING("num=42 and ok (3.5)", tail);

  dstr_destroy(base);
  dstr_destroy(tail);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_create_and_append_should_build_string);
  RUN_TEST(test_concat_should_return_new_combined_string);
  RUN_TEST(test_format_functions_should_write_expected_text);
  return UNITY_END();
}
