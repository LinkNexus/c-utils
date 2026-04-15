#ifndef C_UTILS_DYN_STR_H
#define C_UTILS_DYN_STR_H

#include "utils.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef char *dstr;

typedef struct {
  size_t len;
  size_t cap;
  char buf[];
} dstr_hdr;

#define DSTR_INIT_CAP 16
#define DSTR_HDR(s) ((dstr_hdr *)((char *)(s) - sizeof(dstr_hdr)))

dstr dstr_create(void) {
  dstr_hdr *hdr = xmalloc(sizeof(dstr_hdr) + DSTR_INIT_CAP);
  hdr->len = 0;
  hdr->cap = DSTR_INIT_CAP;
  hdr->buf[0] = '\0';
  return hdr->buf;
}

dstr dstr_from(const char *s) {
  size_t len = strlen(s);
  size_t cap = MAX(DSTR_INIT_CAP, len + 1);

  dstr_hdr *hdr = xmalloc(sizeof(dstr_hdr) + cap);
  hdr->len = len;
  hdr->cap = cap;
  memcpy(hdr->buf, s, len + 1);
  return hdr->buf;
}

void dstr_destroy(dstr s) { free(DSTR_HDR(s)); }

size_t dstr_len(dstr s) { return DSTR_HDR(s)->len; }

char *dstr_to_cstr(const dstr s) { return strndup(s, dstr_len(s)); }

static dstr dstr_reserve(dstr s, size_t new_cap) {
  dstr_hdr *hdr = DSTR_HDR(s);

  if (hdr->cap >= new_cap)
    return s;

  dstr_hdr *new_hdr = xrealloc(hdr, sizeof(dstr_hdr) + new_cap);
  new_hdr->cap = new_cap;
  return new_hdr->buf;
}

static dstr dstr_ensure_cap(dstr s, size_t req_cap) {
  dstr_hdr *hdr = DSTR_HDR(s);

  if (hdr->cap >= req_cap)
    return s;

  size_t new_cap = MAX(hdr->cap * 2, req_cap);
  return dstr_reserve(s, new_cap);
}

void dstr_append(dstr *s, const char *s2) {
  size_t len1 = dstr_len(*s);
  size_t len2 = strlen(s2);
  size_t new_len = len1 + len2;

  *s = dstr_ensure_cap(*s, new_len + 1);
  strcat(*s, s2);
  DSTR_HDR(*s)->len = new_len;
}

dstr dstr_concat(const dstr s1, const dstr s2) {
  dstr res = dstr_create();

  size_t len1 = dstr_len(s1);
  size_t len2 = dstr_len(s2);
  size_t new_len = len1 + len2;

  res = dstr_ensure_cap(res, new_len + 1);
  memcpy(res, s1, len1);
  memcpy(res + len1, s2, len2 + 1);
  DSTR_HDR(res)->len = new_len;
  return res;
}

void dstr_append_char(dstr *s, char c) {
  size_t len = dstr_len(*s);
  size_t new_len = len + 1;

  *s = dstr_ensure_cap(*s, new_len + 1);
  (*s)[len] = c;
  (*s)[new_len] = '\0';
  DSTR_HDR(*s)->len = new_len;
}

dstr dstr_concat_char(const dstr s, char c) {
  dstr res = dstr_create();

  size_t len = dstr_len(s);
  size_t new_len = len + 1;

  res = dstr_ensure_cap(res, new_len + 1);
  memcpy(res, s, len);
  res[len] = c;
  res[new_len] = '\0';
  DSTR_HDR(res)->len = new_len;
  return res;
}

static size_t va_req_len(const char *fmt, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  int required_len = vsnprintf(NULL, 0, fmt, args_copy);

  if (required_len < 0) {
    va_end(args_copy);
    return 0;
  }

  va_end(args_copy);
  return (size_t)required_len;
}

dstr dstr_fmt(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t new_len = va_req_len(fmt, args);

  if (new_len == 0) {
    va_end(args);
    fprintf(stderr, "dstr_format: Error formatting string: '%s'\n", fmt);
    return NULL;
  }

  dstr res = dstr_create();
  res = dstr_ensure_cap(res, new_len + 1);

  vsnprintf(res, new_len + 1, fmt, args);
  DSTR_HDR(res)->len = new_len;

  va_end(args);
  return res;
}

void dstr_append_fmt(dstr *s, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t len1 = dstr_len(*s);
  size_t fmt_len = va_req_len(fmt, args);
  size_t new_len = len1 + fmt_len;

  *s = dstr_ensure_cap(*s, new_len + 1);
  vsnprintf(*s + len1, fmt_len + 1, fmt, args);
  DSTR_HDR(*s)->len = new_len;

  va_end(args);
}

dstr dstr_concat_fmt(const dstr s, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t len1 = dstr_len(s);
  size_t fmt_len = va_req_len(fmt, args);
  size_t new_len = len1 + fmt_len;

  dstr res = dstr_create();
  res = dstr_ensure_cap(res, new_len + 1);

  memcpy(res, s, len1);
  vsnprintf(res + len1, fmt_len + 1, fmt, args);
  DSTR_HDR(res)->len = new_len;

  va_end(args);
  return res;
}

#endif // C_UTILS_DYN_STR_H
