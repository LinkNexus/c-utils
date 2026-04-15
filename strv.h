#ifndef C_UTILS_STRV_H
#define C_UTILS_STRV_H

#include "dstr.h"
#include <stddef.h>
#include <string.h>

typedef struct {
  const char *data;
  size_t len;
} strv;

strv strv_from(const char *str) {
  size_t len = strlen(str);
  return (strv){.data = str, .len = len};
}

strv strv_from_len(const char *str, size_t len) {
  return (strv){.data = str, .len = len};
}

strv strv_from_dstr(const dstr str) {
  dstr_hdr *hdr = DSTR_HDR(str);
  return (strv){.data = str, .len = hdr->len};
}

int strv_cmp(const strv a, const strv b) {
  size_t min_len = a.len < b.len ? a.len : b.len;
  int cmp = memcmp(a.data, b.data, min_len);
  if (cmp != 0) {
    return cmp;
  }
  if (a.len < b.len) {
    return -1;
  } else if (a.len > b.len) {
    return 1;
  } else {
    return 0;
  }
}

bool strv_eq(const strv a, const strv b) { return strv_cmp(a, b) == 0; }

strv strv_substr(const strv str, size_t start, size_t len) {
  if (start > str.len) {
    start = str.len;
  }
  if (start + len > str.len) {
    len = str.len - start;
  }
  return (strv){.data = str.data + start, .len = len};
}

strv strv_slice(const strv str, size_t start) {
  if (start > str.len) {
    start = str.len;
  }
  return (strv){.data = str.data + start, .len = str.len - start};
}

size_t strv_find(const strv str, const strv substr) {
  for (size_t i = 0; i <= str.len - substr.len; i++) {
    if (strv_cmp(strv_substr(str, i, substr.len), substr) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

size_t strv_find_char(const strv str, char c) {
  for (size_t i = 0; i < str.len; i++) {
    if (str.data[i] == c) {
      return i;
    }
  }
  return (size_t)-1;
}

size_t strv_rfind(const strv str, const strv substr) {
  for (size_t i = str.len - substr.len; i != (size_t)-1; i--) {
    if (strv_cmp(strv_substr(str, i, substr.len), substr) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

bool strv_starts_with(const strv str, const strv prefix) {
  if (str.len < prefix.len) {
    return false;
  }
  return strv_cmp(strv_substr(str, 0, prefix.len), prefix) == 0;
}

bool strv_ends_with(const strv str, const strv suffix) {
  if (str.len < suffix.len) {
    return false;
  }
  return strv_cmp(strv_substr(str, str.len - suffix.len, suffix.len), suffix) ==
         0;
}

strv strv_trim_left(const strv str) {
  size_t start = 0;
  while (start < str.len &&
         (str.data[start] == ' ' || str.data[start] == '\t')) {
    start++;
  }
  return strv_substr(str, start, str.len - start);
}

strv strv_trim_right(const strv str) {
  size_t end = str.len;
  while (end > 0 && (str.data[end - 1] == ' ' || str.data[end - 1] == '\t')) {
    end--;
  }
  return strv_substr(str, 0, end);
}

strv strv_trim(const strv str) { return strv_trim_right(strv_trim_left(str)); }

#endif // !DEBUG
