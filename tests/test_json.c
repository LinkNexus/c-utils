#include "../json.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>

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

  fclose(f);
  return 0;
}
