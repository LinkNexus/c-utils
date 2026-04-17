#include "../json.h"
#include "darr.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>

static void print_obj(const HashTableEntry* entry, size_t idx);

static void print_arr(const void* el, size_t idx) {
  JsonValue* value = (JsonValue*)el;
  printf("[%zu]: ", idx);
  switch (value->type) {
    case JSON_NULL:
      printf("null");
      break;
    case JSON_BOOL:
      printf(value->bool_value ? "true" : "false");
      break;
    case JSON_NUMBER:
      printf("%g", value->number_value);
      break;
    case JSON_STRING:
      printf("\"%s\"", value->string_value);
      break;
    case JSON_ARRAY:
      printf("[array with %zu elements]\n", value->array_value.size);
      darr_iterate(&value->array_value, print_arr);
      break;
    case JSON_OBJECT:
      printf("{object with %zu properties}\n", value->object_value.size);
      ht_iterate(&value->object_value, print_obj);
      break;
  }
  printf("\n");
}

static void print_obj(const HashTableEntry* entry, size_t idx) {
  JsonValue* value = entry->value;
  printf("%s: ", entry->key);
  switch (value->type) {
    case JSON_NULL:
      printf("null");
      break;
    case JSON_BOOL:
      printf(value->bool_value ? "true" : "false");
      break;
    case JSON_NUMBER:
      printf("%g", value->number_value);
      break;
    case JSON_STRING:
      printf("\"%s\"", value->string_value);
      break;
    case JSON_ARRAY:
      printf("[array with %zu elements]\n", value->array_value.size);
      darr_iterate(&value->array_value, print_arr);
      break;
    case JSON_OBJECT:
      printf("{object with %zu properties}\n", value->object_value.size);
      ht_iterate(&value->object_value, print_obj);
      break;
  }
  printf("\n");
}

int main(void) {
  JsonValue json;
  char* err;

  FILE* f = fopen("test.json", "rb");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);

  char* buffer = xmalloc(fsize + 1);
  fread(buffer, 1, fsize, f);
  buffer[fsize] = '\0';

  bool res = json_parse(buffer, &json, &err);

  if (!res) {
    fprintf(stderr, "Error parsing JSON: %s\n", err);
    free(buffer);
    fclose(f);
    return 1;
  }

  JsonValue* name_val;
  res = json_get(&json, "name", &name_val);
  assert(name_val->type == JSON_STRING);

  // ht_iterate(&json.object_value, print_obj);

  printf("Json contents: \n%s", json_stringify(&json, 2));

  json_free(&json);
  fclose(f);
  return 0;
}
