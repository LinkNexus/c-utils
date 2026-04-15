#include "../ht.h"
#include <stdio.h>

void print_entry(const ht_entry *entry, size_t idx) {
  printf("Index: %zu, Key: %s, Value: %d\n", idx, entry->key,
         *(int *)entry->value);
}

int main(void) {
  ht table = ht_create(sizeof(int));

  ht_set(&table, "one", &(int){1});
  ht_set(&table, "two", &(int){2});
  ht_set(&table, "one", &(int){11});

  ht_delete(&table, "two");

  ht_iterate(&table, print_entry);

  ht_destroy(&table);
  return 0;
}
