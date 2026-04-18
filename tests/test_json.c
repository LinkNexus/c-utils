#include "json.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_parse_primitive_values_should_fill_json_value(void) {
  JsonValue out = {0};
  char* err = NULL;

  TEST_ASSERT_TRUE(json_parse("123", &out, &err));
  TEST_ASSERT_EQUAL_INT(JSON_NUMBER, out.type);
  TEST_ASSERT_DOUBLE_WITHIN(0.0001, 123.0, out.number_value);

  TEST_ASSERT_TRUE(json_free(&out));
  TEST_ASSERT_NULL(err);
}

void test_parse_object_and_array_should_return_structured_value(void) {
  const char* src = "{\"name\":\"bob\",\"nums\":[1,2,3],\"ok\":true}";
  JsonValue out = {0};
  char* err = NULL;

  TEST_ASSERT_TRUE(json_parse(src, &out, &err));
  TEST_ASSERT_EQUAL_INT(JSON_OBJECT, out.type);
  TEST_ASSERT_EQUAL_UINT64(3, out.object_value.size);

  JsonValue* name = (JsonValue*)ht_get(&out.object_value, "name");
  JsonValue* nums = (JsonValue*)ht_get(&out.object_value, "nums");
  JsonValue* ok = (JsonValue*)ht_get(&out.object_value, "ok");

  TEST_ASSERT_NOT_NULL(name);
  TEST_ASSERT_NOT_NULL(nums);
  TEST_ASSERT_NOT_NULL(ok);
  TEST_ASSERT_EQUAL_INT(JSON_STRING, name->type);
  TEST_ASSERT_EQUAL_STRING("bob", name->string_value);
  TEST_ASSERT_EQUAL_INT(JSON_ARRAY, nums->type);
  TEST_ASSERT_EQUAL_UINT64(3, nums->array_value.size);
  TEST_ASSERT_EQUAL_INT(JSON_BOOL, ok->type);
  TEST_ASSERT_TRUE(ok->bool_value);

  TEST_ASSERT_TRUE(json_free(&out));
  TEST_ASSERT_NULL(err);
}

void test_stringify_should_escape_and_format_string_values(void) {
  JsonValue value = {
      .type = JSON_STRING,
      .string_value = strdup("hello\\n\"x\""),
  };

  dstr out = json_stringify(&value, -1);

  TEST_ASSERT_EQUAL_STRING("\"hello\\\\n\\\"x\\\"\"", out);

  dstr_destroy(out);
  free(value.string_value);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_primitive_values_should_fill_json_value);
  RUN_TEST(test_parse_object_and_array_should_return_structured_value);
  RUN_TEST(test_stringify_should_escape_and_format_string_values);
  return UNITY_END();
}
