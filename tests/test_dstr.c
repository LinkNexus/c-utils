#include "../dstr.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
  dstr s = dstr_from("Hello, world!");
  printf("s: %s\n", s);

  dstr s2 = dstr_create();
  dstr_append(&s2, "Hello, ");
  dstr_append(&s2, "world!");
  printf("s2: %s\n", s2);

  dstr s3 = dstr_concat(s, s2);
  printf("s3: %s\n", s3);

  dstr s4 = dstr_fmt("Formatted number: %d", 42);
  dstr_append_fmt(&s4, ", hex: %x", 42);
  printf("s4: %s\n", s4);

  dstr s5 = dstr_concat_fmt(s4, ", octal: %o", 42);
  printf("s5: %s\n", s5);

  dstr_destroy(s);
  dstr_destroy(s2);
  dstr_destroy(s3);
  dstr_destroy(s4);
  dstr_destroy(s5);

  return EXIT_SUCCESS;
}
