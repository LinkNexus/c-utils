#include "../dyn_arr.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void print_el(void *el, size_t idx) {
  printf("El at idx %zu is %d\n", idx, *(int *)el);
}

bool is_even(void *el, size_t idx) { return *(int *)el % 2 == 0; }

int main(int argc, char *argv[]) {
  DynArr *arr = dyn_arr_create(sizeof(int));
  printf("Vector created with element size: %zu, capacity: %zu\n", arr->el_size,
         arr->capacity);

  dyn_arr_push_back(arr, &(int){1});
  dyn_arr_push_back(arr, &(int){2});
  dyn_arr_push_back(arr, &(int){3});
  dyn_arr_push_back(arr, &(int){4});

  dyn_arr_pop_back(arr);

  dyn_arr_insert_at(arr, 0, &(int){10});
  dyn_arr_insert_at(arr, 2, &(int){100});

  dyn_arr_remove_at(arr, 1);

  dyn_arr_set(arr, 0, &(int){-1});

  dyn_arr_iterate(arr, print_el);

  DynArr *filtered_arr = dyn_arr_filter(arr, is_even);
  dyn_arr_iterate(filtered_arr, print_el);

  return EXIT_SUCCESS;
}
