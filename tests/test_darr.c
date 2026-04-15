#include "../darr.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void print_el(void *el, size_t idx) {
  printf("El at idx %zu is %d\n", idx, *(int *)el);
}

bool is_even(void *el, size_t idx) { return *(int *)el % 2 == 0; }

int main(int argc, char *argv[]) {
  darr arr = darr_create(sizeof(int));
  printf("Vector created with element size: %zu, capacity: %zu\n", arr.el_size,
         arr.capacity);

  darr_push_back(&arr, &(int){1});
  darr_push_back(&arr, &(int){2});
  darr_push_back(&arr, &(int){3});
  darr_push_back(&arr, &(int){4});

  darr_pop_back(&arr);

  darr_insert_at(&arr, 0, &(int){10});
  darr_insert_at(&arr, 2, &(int){100});

  darr_remove_at(&arr, 1);

  darr_set(&arr, 0, &(int){-1});

  darr_iterate(&arr, print_el);

  darr filtered_arr = darr_filter(&arr, is_even);
  darr_iterate(&filtered_arr, print_el);

  return EXIT_SUCCESS;
}
