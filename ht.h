#ifndef C_UTILS_H_TABLE_H
#define C_UTILS_H_TABLE_H

#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#define H_TABLE_MIN 4
#define H_TABLE_LOAD_FACTOR 0.75

typedef struct {
  char* key;
  void* value;
  bool is_deleted;
} HashTableEntry;

typedef struct {
  HashTableEntry* entries;
  size_t capacity;
  size_t size;
  size_t el_size;
  void (*val_destructor)(void* value);
} HashTable;

static unsigned long ht_hash_key(const char* str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;

  return hash;
}

HashTable ht_create(size_t el_size) {
  return (HashTable){
      .capacity = H_TABLE_MIN,
      .size = 0,
      .el_size = el_size,
      .entries = xcalloc(H_TABLE_MIN, sizeof(HashTableEntry)),
      .val_destructor = NULL,
  };
}

void ht_destroy(HashTable* table) {
  for (size_t i = 0; i < table->capacity; ++i) {
    HashTableEntry* entry = &table->entries[i];

    if (table->val_destructor && entry->key && !entry->is_deleted)
      table->val_destructor(entry->value);

    free(entry->key);
    free(entry->value);
  }

  free(table->entries);
}

static void ht_resize(HashTable* table, size_t new_capacity) {
  new_capacity = MAX(new_capacity, H_TABLE_MIN);

  if (new_capacity == table->capacity)
    return;

  HashTableEntry* new_entries = xcalloc(new_capacity, sizeof *new_entries);

  for (size_t i = 0; i < table->capacity; ++i) {
    HashTableEntry* entry = &table->entries[i];

    if (entry->key) {
      size_t idx = ht_hash_key(entry->key) % new_capacity;

      while (new_entries[idx].key) {
        idx = (idx + 1) % new_capacity;
      }

      new_entries[idx].key = entry->key;
    }
  }

  free(table->entries);
  table->entries = new_entries;
  table->capacity = new_capacity;
}

static void ht_ensure_capacity(HashTable* table) {
  if (table->capacity && (float)table->size / table->capacity < H_TABLE_LOAD_FACTOR)
    return;

  size_t new_capacity = table->capacity ? table->capacity * 2 : H_TABLE_MIN;

  ht_resize(table, new_capacity);
}

void ht_set(HashTable* table, const char* key, const void* value) {
  ht_ensure_capacity(table);
  size_t idx = ht_hash_key(key) % table->capacity;

  size_t checked = 0;
  size_t first_deleted_idx = -1;

  while ((table->entries[idx].key || table->entries[idx].is_deleted) && checked < table->capacity) {
    if (table->entries[idx].is_deleted && first_deleted_idx == (size_t)-1)
      first_deleted_idx = idx;
    else if (!table->entries[idx].is_deleted && strcmp(table->entries[idx].key, key) == 0) {
      if (table->val_destructor)
        table->val_destructor(table->entries[idx].value);

      memcpy(table->entries[idx].value, value, table->el_size);
      return;
    }

    idx = (idx + 1) % table->capacity;
    checked++;
  }

  if (first_deleted_idx != (size_t)-1)
    idx = first_deleted_idx;

  table->entries[idx].key = strdup(key);
  table->entries[idx].value = xmalloc(table->el_size);
  table->entries[idx].is_deleted = false;

  memcpy(table->entries[idx].value, value, table->el_size);
  table->size++;
}

void ht_iterate(const HashTable* table, void (*fn)(const HashTableEntry* entry, size_t idx)) {
  for (size_t i = 0; i < table->capacity; ++i) {
    if (table->entries[i].key && !table->entries[i].is_deleted) {
      fn(&table->entries[i], i);
    }
  }
}

void* ht_get(const HashTable* table, const char* key) {
  if (!table->capacity)
    return NULL;

  size_t idx = ht_hash_key(key) % table->capacity;
  size_t checked = 0;

  while ((table->entries[idx].key || table->entries[idx].is_deleted) && checked < table->capacity) {
    if (!table->entries[idx].is_deleted && strcmp(table->entries[idx].key, key) == 0)
      return table->entries[idx].value;

    idx = (idx + 1) % table->capacity;
    checked++;
  }

  return NULL;
}

void ht_delete(HashTable* table, const char* key) {
  if (!table->capacity)
    return;

  size_t idx = ht_hash_key(key) % table->capacity;
  size_t checked = 0;

  while ((table->entries[idx].key || table->entries[idx].is_deleted) && checked < table->capacity) {
    if (strcmp(table->entries[idx].key, key) == 0) {
      if (table->val_destructor)
        table->val_destructor(table->entries[idx].value);

      free(table->entries[idx].key);
      free(table->entries[idx].value);

      table->entries[idx].key = NULL;
      table->entries[idx].value = NULL;
      table->entries[idx].is_deleted = true;
      table->size--;
      return;
    }

    idx = (idx + 1) % table->capacity;
    checked++;
  }
}

#endif // !DEBUG
