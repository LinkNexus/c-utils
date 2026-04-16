#include "../json.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  const char* json_str = "{\"name\": \"Alice\", \"age\": 30, \"is_student\": false, \"skills\": "
                         "[\"C\", \"Python\" \"JavaScript\"]}";

  printf("Testing JSON parsing with invalid input...\n");

  JsonValue json_val;
  char* err_msg;
  //
  bool res = json_parse(json_str, &json_val, &err_msg);

  if (res) {
    fprintf(stderr, "Expected parsing to fail, but it succeeded\n");
  } else {
    fprintf(stderr, "Erro when parsing: %s\n", err_msg);
  }

  return 0;
}

// int main(void) {
//   const char *json_str = "{\"name\": \"Alice\", \"age\": 30, \"is_student\":
//   false}"; JsonValue *json = json_parse(json_str); if (json == NULL) {
//     fprintf(stderr, "Failed to parse JSON\n");
//     return 1;
//   }
//
//   // Check that the parsed JSON is an object
//   if (json->type != JSON_OBJECT) {
//     fprintf(stderr, "Expected JSON object\n");
//     return 1;
//   }
//
//   // Check that the object has the expected keys and values
//   HTable *obj = json->object_value;
//
//   JsonValue *name_val = htable_get(obj, "name");
//   if (name_val == NULL || name_val->type != JSON_STRING ||
//   strcmp(name_val->string_value, "Alice") != 0) {
//     fprintf(stderr, "Expected name to be 'Alice'\n");
//     return 1;
//   }
//
//   JsonValue *age_val = htable_get(obj, "age");
//   if (age_val == NULL || age_val->type != JSON_NUMBER ||
//   age_val->number_value != 30) {
//     fprintf(stderr, "Expected age to be 30\n");
//     return 1;
//   }
//
//   JsonValue *is_student_val = htable_get(obj, "is_student");
//   if (is_student_val == NULL || is_student_val->type != JSON_BOOL ||
//   is_student_val->bool_value != false) {
//     fprintf(stderr, "Expected is_student to be false\n");
//     return 1;
//   }
//
//   printf("JSON parsed successfully!\n");
//
//   // Clean up
//   json_free(json);
//
//   return 0;
// }
