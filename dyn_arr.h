#ifndef C_UTILS_DYN_ARR_H
#define C_UTILS_DYN_ARR_H

#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DYN_ARR_MIN 4

#include <stddef.h>

typedef struct {
  void *items;
  size_t el_size;
  size_t size;
  size_t capacity;
  void (*val_destructor)(void *val);
} DynArr;

DynArr *dyn_arr_create(size_t el_size) {
  DynArr *arr = xmalloc(sizeof *arr);
  arr->el_size = el_size;
  arr->size = 0;
  arr->capacity = DYN_ARR_MIN;
  arr->items = xmalloc(arr->capacity * arr->el_size);
  arr->val_destructor = NULL;
  return arr;
}

void *dyn_arr_get(const DynArr *arr, size_t idx) {
  if (idx < 0 || idx >= arr->size) {
    fprintf(stderr, "Error: index out of bounds\n");
    return NULL;
  }
  return (char *)arr->items + arr->el_size * idx;
}

void dyn_arr_destroy(DynArr *arr) {
  if (arr->val_destructor) {
    for (size_t i = 0; i < arr->size; ++i) {
      arr->val_destructor(dyn_arr_get(arr, i));
    }
  }
  free(arr->items);
  free(arr);
}

static bool dyn_arr_reserve(DynArr *arr, size_t min_capacity) {
  if (arr->capacity >= min_capacity)
    return true;

  size_t new_capacity = MAX(arr->capacity, DYN_ARR_MIN);

  if (new_capacity > SIZE_MAX / arr->el_size) {
    fprintf(stderr, "Error: capacity overflow\n");
    return false;
  }

  arr->items = xrealloc(arr->items, new_capacity * arr->el_size);
  arr->capacity = new_capacity;

  return true;
}

static bool dyn_arr_ensure_capacity(DynArr *arr, size_t required_size) {
  if (arr->capacity >= required_size)
    return true;

  size_t new_capacity = MAX(MAX(arr->capacity * 2, required_size), DYN_ARR_MIN);

  return dyn_arr_reserve(arr, new_capacity);
}

bool dyn_arr_push_back(DynArr *arr, const void *el) {
  if (!dyn_arr_ensure_capacity(arr, arr->size + 1)) {
    return false;
  }

  memcpy((char *)arr->items + arr->el_size * arr->size, el, arr->el_size);
  arr->size += 1;
  return true;
}

void dyn_arr_iterate(const DynArr *arr, void (*fn)(void *el, size_t idx)) {
  for (size_t i = 0; i < arr->size; ++i) {
    fn(dyn_arr_get(arr, i), i);
  }
}

void dyn_arr_adjust_capacity(DynArr *arr, size_t new_size) {
  if (new_size >= arr->capacity)
    return;

  if (new_size < arr->capacity / 4) {
    size_t new_capacity = MAX(arr->capacity / 2, DYN_ARR_MIN);
    arr->items = xrealloc(arr->items, new_capacity * arr->el_size);
    arr->capacity = new_capacity;
  }
}

void dyn_arr_pop_back(DynArr *arr) {
  if (arr->size == 0)
    return;
  arr->size = arr->size - 1;
  if (arr->val_destructor)
    arr->val_destructor((char *)arr->items + arr->el_size * arr->size);

  dyn_arr_adjust_capacity(arr, arr->size);
}

void dyn_arr_set(DynArr *arr, size_t idx, const void *el) {
  if (idx < 0 || idx >= arr->size)
    fprintf(stderr, "Error: index out of bounds\n");
  memcpy(dyn_arr_get(arr, idx), el, arr->el_size);
}

bool dyn_arr_insert_at(DynArr *arr, size_t idx, const void *el) {
  if (idx < 0 || idx >= arr->size) {
    fprintf(stderr, "Error: index out of bounds\n");
    return false;
  }

  if (!dyn_arr_ensure_capacity(arr, arr->size + 1))
    return false;

  memmove(dyn_arr_get(arr, idx + 1), dyn_arr_get(arr, idx),
          (arr->size - idx) * arr->el_size);
  memmove(dyn_arr_get(arr, idx), el, arr->el_size);

  arr->size = arr->size + 1;
  return true;
}

bool dyn_arr_remove_at(DynArr *arr, size_t idx) {
  if (idx < 0 || idx >= arr->size) {
    fprintf(stderr, "Error: index out of bounds\n");
    return false;
  }

  if (arr->val_destructor)
    arr->val_destructor(dyn_arr_get(arr, idx));

  memmove(dyn_arr_get(arr, idx), dyn_arr_get(arr, idx + 1),
          (arr->size - idx - 1) * arr->el_size);

  arr->size = arr->size - 1;

  dyn_arr_adjust_capacity(arr, arr->size);
  return true;
}

bool dyn_arr_append_range(DynArr *arr, const void *el, size_t count) {
  if (!dyn_arr_ensure_capacity(arr, arr->size + count))
    return false;

  memmove((char *)arr->items + (arr->el_size * arr->size), el,
          count * arr->el_size);
  arr->size = arr->size + count;

  return true;
}

DynArr *dyn_arr_clone(const DynArr *arr) {
  size_t total_size = arr->size * arr->el_size;
  DynArr *clone = dyn_arr_create(arr->el_size);

  clone->items = xmalloc(arr->capacity * arr->el_size);
  memcpy(clone->items, arr->items, total_size);
  clone->size = arr->size;
  clone->capacity = arr->capacity;

  return clone;
}

DynArr *dyn_arr_filter(DynArr *arr, bool (*predicate)(void *el, size_t idx)) {
  DynArr *new_arr = dyn_arr_create(arr->el_size);

  for (size_t i = 0; i < arr->size; ++i) {
    void *el = dyn_arr_get(arr, i);
    if (predicate(el, i)) {
      dyn_arr_push_back(new_arr, el);
    }
  }

  return new_arr;
}

#endif // !DEBUG
