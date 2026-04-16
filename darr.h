#ifndef C_UTILS_DYN_ARR_H
#define C_UTILS_DYN_ARR_H

#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DYN_ARR_MIN 4

#include <stddef.h>

typedef struct {
  void* items;
  size_t el_size;
  size_t size;
  size_t capacity;
  void (*val_destructor)(void* val);
} Darr;

Darr darr_create(size_t el_size) {
  return (Darr){
      .el_size = el_size,
      .size = 0,
      .capacity = DYN_ARR_MIN,
      .items = xmalloc(DYN_ARR_MIN * el_size),
      .val_destructor = NULL,
  };
}

void* darr_get(const Darr* arr, size_t idx) {
  if (idx < 0 || idx >= arr->size) {
    fprintf(stderr, "Error: index out of bounds\n");
    return NULL;
  }
  return (char*)arr->items + arr->el_size * idx;
}

void darr_destroy(Darr* arr) {
  if (arr->val_destructor) {
    for (size_t i = 0; i < arr->size; ++i) {
      arr->val_destructor(darr_get(arr, i));
    }
  }
  free(arr->items);
  arr->items = NULL;
  arr->size = 0;
  arr->capacity = 0;
}

static bool darr_reserve(Darr* arr, size_t min_capacity) {
  if (arr->capacity >= min_capacity)
    return true;

  size_t new_capacity = MAX(arr->capacity, min_capacity);

  if (new_capacity > SIZE_MAX / arr->el_size) {
    fprintf(stderr, "Error: capacity overflow\n");
    return false;
  }

  arr->items = xrealloc(arr->items, new_capacity * arr->el_size);
  arr->capacity = new_capacity;

  return true;
}

static bool darr_ensure_capacity(Darr* arr, size_t required_size) {
  if (arr->capacity >= required_size)
    return true;

  size_t new_capacity = MAX(MAX(arr->capacity * 2, required_size), DYN_ARR_MIN);

  return darr_reserve(arr, new_capacity);
}

bool darr_push_back(Darr* arr, const void* el) {
  if (!darr_ensure_capacity(arr, arr->size + 1)) {
    return false;
  }

  memcpy((char*)arr->items + arr->el_size * arr->size, el, arr->el_size);
  arr->size += 1;
  return true;
}

void darr_iterate(const Darr* arr, void (*fn)(void* el, size_t idx)) {
  for (size_t i = 0; i < arr->size; ++i) {
    fn(darr_get(arr, i), i);
  }
}

void darr_adjust_capacity(Darr* arr, size_t new_size) {
  if (new_size >= arr->capacity)
    return;

  if (new_size < arr->capacity / 4) {
    size_t new_capacity = MAX(arr->capacity / 2, DYN_ARR_MIN);
    arr->items = xrealloc(arr->items, new_capacity * arr->el_size);
    arr->capacity = new_capacity;
  }
}

void darr_pop_back(Darr* arr) {
  if (arr->size == 0)
    return;
  arr->size = arr->size - 1;
  if (arr->val_destructor)
    arr->val_destructor((char*)arr->items + arr->el_size * arr->size);

  darr_adjust_capacity(arr, arr->size);
}

void darr_set(Darr* arr, size_t idx, const void* el) {
  if (idx < 0 || idx >= arr->size)
    fprintf(stderr, "Error: index out of bounds\n");
  memcpy(darr_get(arr, idx), el, arr->el_size);
}

bool darr_insert_at(Darr* arr, size_t idx, const void* el) {
  if (idx < 0 || idx >= arr->size) {
    fprintf(stderr, "Error: index out of bounds\n");
    return false;
  }

  if (!darr_ensure_capacity(arr, arr->size + 1))
    return false;

  memmove(darr_get(arr, idx + 1), darr_get(arr, idx), (arr->size - idx) * arr->el_size);
  memmove(darr_get(arr, idx), el, arr->el_size);

  arr->size = arr->size + 1;
  return true;
}

bool darr_remove_at(Darr* arr, size_t idx) {
  if (idx < 0 || idx >= arr->size) {
    fprintf(stderr, "Error: index out of bounds\n");
    return false;
  }

  if (arr->val_destructor)
    arr->val_destructor(darr_get(arr, idx));

  memmove(darr_get(arr, idx), darr_get(arr, idx + 1), (arr->size - idx - 1) * arr->el_size);

  arr->size = arr->size - 1;

  darr_adjust_capacity(arr, arr->size);
  return true;
}

bool darr_append_range(Darr* arr, const void* el, size_t count) {
  if (!darr_ensure_capacity(arr, arr->size + count))
    return false;

  memmove((char*)arr->items + (arr->el_size * arr->size), el, count * arr->el_size);
  arr->size = arr->size + count;

  return true;
}

Darr darr_clone(const Darr* arr) {
  size_t total_size = arr->size * arr->el_size;
  Darr clone = darr_create(arr->el_size);

  clone.items = xmalloc(arr->capacity * arr->el_size);
  memcpy(clone.items, arr->items, total_size);
  clone.size = arr->size;
  clone.capacity = arr->capacity;

  return clone;
}

Darr darr_filter(Darr* arr, bool (*predicate)(void* el, size_t idx)) {
  Darr new_arr = darr_create(arr->el_size);

  for (size_t i = 0; i < arr->size; ++i) {
    void* el = darr_get(arr, i);
    if (predicate(el, i)) {
      darr_push_back(&new_arr, el);
    }
  }

  return new_arr;
}

#endif // !DEBUG
