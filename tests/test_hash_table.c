#include "../hash_table.h"
#include <stdio.h>

void print_entry(const HTableEntry *entry, size_t idx) {
  printf("Index: %zu, Key: %s, Value: %d\n", idx, entry->key,
         *(int *)entry->value);
}

int main(void) {
  HTable *table = h_table_create(sizeof(int));

  h_table_set(table, "one", &(int){1});
  h_table_set(table, "two", &(int){2});
  h_table_set(table, "one", &(int){11});

  h_table_delete(table, "two");

  h_table_iterate(table, print_entry);

  h_table_destroy(table);
  return 0;
}
