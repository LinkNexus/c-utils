#include "../ht.h"
#include <stdio.h>

void print_entry(const HashTableEntry* entry, size_t idx) {
  printf("Index: %zu, Key: %s, Value: %d\n", idx, entry->key, *(int*)entry->value);
}

int main(void) {
  HashTable table = ht_create(sizeof(int));

  ht_set(&table, "one", &(int){1});
  ht_set(&table, "two", &(int){2});
  ht_set(&table, "one", &(int){11});
  ht_set(&table, "three", &(int){3});
  ht_set(&table, "three", &(int){33});

  ht_delete(&table, "two");

  ht_iterate(&table, print_entry);

  ht_destroy(&table);
  return 0;
}
