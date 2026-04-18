# c-utils

A small, header-only C utility library with tests.

This repository provides reusable data structures and string/JSON helpers in plain C11:

- Dynamic arrays (`darr.h`)
- Dynamic strings (`dstr.h`)
- Hash table (`ht.h`)
- String views and split helpers (`strv.h`)
- JSON parser and serializer (`json.h`)
- Allocation and math helpers (`utils.h`)

## Highlights

- Header-only modules (drop-in includes)
- Generic dynamic array and hash table APIs (store any element type by size)
- Lightweight string view utilities (`strv`) with split and trim operations
- JSON parse + stringify support (objects, arrays, strings, numbers, booleans, null)
- Unity-based unit tests with CMake and Make workflows

## Repository Layout

- `darr.h`: generic dynamic array
- `dstr.h`: heap-backed dynamic string type (`dstr`)
- `ht.h`: linear-probing hash table with string keys
- `strv.h`: non-owning string views and split/find/trim helpers
- `json.h`: JSON lexer/parser, path access, and stringify
- `utils.h`: `xmalloc`/`xrealloc`/`xcalloc`, `MIN`/`MAX`/`ABS`
- `tests/`: unit tests for each module

## Requirements

- C11 compiler
- CMake >= 3.16 (for CMake flow)
- Make (optional, convenience wrapper)

## Build and Test

### Option 1: Make (recommended quick path)

```bash
make test
```

Useful targets:

```bash
make build
make test-verbose
make test-one NAME=test_dstr
make clean
```

### Option 2: CMake directly

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Using in Your Project

These headers contain full function definitions. In a multi-file program, avoid including the same implementation header in multiple translation units unless you are intentionally using this pattern and understand duplicate symbol rules for your toolchain.

A practical approach is:

- include implementation headers from one `.c` file, then
- expose your own wrapper API in your project headers as needed.

For quick single-file tools, you can directly include the headers.

## API Overview

### `darr.h` (dynamic array)

Core type:

```c
typedef struct {
  void* items;
  size_t el_size;
  size_t size;
  size_t capacity;
  void (*val_destructor)(void* val);
} Darr;
```

Common operations:

- `darr_create(el_size)`
- `darr_push_back(&arr, &value)`
- `darr_get(&arr, idx)`
- `darr_insert_at(&arr, idx, &value)`
- `darr_remove_at(&arr, idx)`
- `darr_pop_back(&arr)`
- `darr_filter(&arr, predicate)`
- `darr_destroy(&arr)`

Example:

```c
#include "darr.h"

int main(void) {
  Darr arr = darr_create(sizeof(int));
  int a = 10, b = 20;

  darr_push_back(&arr, &a);
  darr_push_back(&arr, &b);

  int* second = (int*)darr_get(&arr, 1);
  /* second points to 20 */

  darr_destroy(&arr);
  return 0;
}
```

### `dstr.h` (dynamic string)

Core type:

```c
typedef char* dstr;
```

Common operations:

- `dstr_create()`, `dstr_from("...")`, `dstr_destroy(s)`
- `dstr_len(s)`
- `dstr_append(&s, "...")`, `dstr_append_char(&s, 'x')`
- `dstr_concat(a, b)`
- `dstr_fmt("num=%d", 42)`
- `dstr_append_fmt(&s, "...", ...)`
- `dstr_concat_fmt(s, "...", ...)`

Example:

```c
#include "dstr.h"

int main(void) {
  dstr s = dstr_create();
  dstr_append(&s, "hello");
  dstr_append_char(&s, ' ');
  dstr_append_fmt(&s, "%s", "world");

  /* s == "hello world" */

  dstr_destroy(s);
  return 0;
}
```

### `ht.h` (hash table)

Core type:

```c
typedef struct {
  HashTableEntry* entries;
  size_t capacity;
  size_t size;
  size_t el_size;
  size_t deleted_count;
  void (*val_destructor)(void* value);
} HashTable;
```

Common operations:

- `ht_create(el_size)`
- `ht_set(&table, "key", &value)`
- `ht_get(&table, "key")`
- `ht_delete(&table, "key")`
- `ht_iterate(&table, callback)`
- `ht_destroy(&table)`

Example:

```c
#include "ht.h"

int main(void) {
  HashTable table = ht_create(sizeof(int));

  int value = 123;
  ht_set(&table, "alpha", &value);

  int* got = (int*)ht_get(&table, "alpha");
  /* got points to 123 */

  ht_destroy(&table);
  return 0;
}
```

### `strv.h` (string view)

Core type:

```c
typedef struct {
  const char* data;
  size_t len;
} strv;
```

Common operations:

- `strv_from("...")`, `strv_from_len(ptr, len)`
- `strv_trim(v)`, `strv_substr(v, start, len)`, `strv_slice(v, start)`
- `strv_find(v, sub)`, `strv_find_char(v, c)`, `strv_rfind(v, sub)`
- `strv_starts_with(v, prefix)`, `strv_ends_with(v, suffix)`
- `strv_split(v, ',')`, `strv_split_str(v, delim)`, `strv_split_any(v, charset)`

Example:

```c
#include "strv.h"

int main(void) {
  strv input = strv_from("a,b,c");
  Darr parts = strv_split(input, ',');

  /* parts contains 3 strv elements: "a", "b", "c" */

  darr_destroy(&parts);
  return 0;
}
```

### `json.h` (JSON)

Core operations:

- `json_parse(input, &root, &err_msg)`
- `json_get(&root, "path.to.value", &out)`
- `json_stringify(&root, indent)` where `indent < 0` means compact output
- `json_free(&root)`

Example:

```c
#include "json.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  const char* src = "{\"name\":\"bob\",\"nums\":[1,2,3],\"ok\":true}";
  JsonValue root = {0};
  char* err = NULL;

  if (!json_parse(src, &root, &err)) {
    fprintf(stderr, "%s\n", err);
    free(err);
    return 1;
  }

  JsonValue* name = (JsonValue*)ht_get(&root.object_value, "name");
  if (name && name->type == JSON_STRING) {
    printf("name=%s\n", name->string_value);
  }

  dstr pretty = json_stringify(&root, 0);
  printf("%s\n", pretty);

  dstr_destroy(pretty);
  json_free(&root);
  return 0;
}
```

## Running Individual Tests

Available test binaries (via CTest):

- `test_darr`
- `test_dstr`
- `test_ht`
- `test_strv`
- `test_json`

Run one test:

```bash
make test-one NAME=test_json
```

## Notes

- The library is focused on small utility use cases and tests currently cover core happy paths.
- `json_get` supports dotted object paths and numeric array segments.
- Some modules provide optional value destructors (`val_destructor`) for owned element cleanup.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
