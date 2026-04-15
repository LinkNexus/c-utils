#include "../strv.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  strv s = strv_from("Hello, World!");
  printf("String: %s, Length: %zu\n", s.data, s.len);

  return EXIT_SUCCESS;
}
