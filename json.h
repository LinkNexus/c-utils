#ifndef C_UTILS_JSON_H
#define C_UTILS_JSON_H

#include "darr.h"
#include "dstr.h"
#include "ht.h"
#include "strv.h"
#include "utils.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef enum { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } JsonType;

typedef enum {
  JSON_TOKEN_STRING, // 0
  JSON_TOKEN_NUMBER,
  JSON_TOKEN_TRUE,
  JSON_TOKEN_FALSE,
  JSON_TOKEN_NULL,
  JSON_TOKEN_LBRACE,
  JSON_TOKEN_RBRACE,
  JSON_TOKEN_LBRACKET,
  JSON_TOKEN_RBRACKET,
  JSON_TOKEN_COLON,
  JSON_TOKEN_COMMA,
  JSON_TOKEN_EOF,
} JsonTokenType;

typedef struct {
  size_t start;
  size_t len;
  size_t start_line;
  size_t start_col;
} TextSpan;

typedef struct {
  char* msg;
  TextSpan span;
} JsonErr;

typedef struct {
  JsonType type;
  char* id;
  TextSpan span;
  union {
    bool bool_value;
    double number_value;
    char* string_value;
    Darr array_value;
    HashTable object_value;
  };
} JsonValue;

typedef struct {
  const char* input;
  JsonErr* err;
  size_t input_len;
  size_t pos;
  size_t col;
  size_t line;
} LexerCtx;

typedef struct {
  JsonTokenType type;
  char* value;
  TextSpan span;
} JsonToken;

typedef struct {
  Darr* tokens;
  size_t pos;
  JsonErr* err;
} ParserCtx;

static bool is_end(LexerCtx* ctx) {
  return ctx->pos >= ctx->input_len;
}

static char peek(LexerCtx* ctx, size_t offset) {
  size_t idx = ctx->pos + offset;
  if (idx >= ctx->input_len)
    return '\0';
  return ctx->input[idx];
}

static void advance(LexerCtx* ctx) {
  if (is_end(ctx))
    return;

  ctx->pos++;
  ctx->col++;
}

static void advance_line(LexerCtx* ctx) {
  if (peek(ctx, 0) == '\r') {
    advance(ctx);

    if (peek(ctx, 0) == '\n')
      advance(ctx);
  } else if (peek(ctx, 0) == '\n') {
    advance(ctx);
  }

  ctx->line++;
  ctx->col = 1;
}

static void skip_trivial_chars(LexerCtx* ctx) {
  while (!is_end(ctx)) {
    char c = peek(ctx, 0);

    switch (c) {
      case ' ':
      case '\t':
        advance(ctx);
        break;

      case '\n':
      case '\r':
        advance_line(ctx);
        break;

      default:
        return;
    }
  }
}

static TextSpan capture_start(LexerCtx* ctx) {
  return (TextSpan){.start = ctx->pos, .len = 0, .start_line = ctx->line, .start_col = ctx->col};
}

static TextSpan capture_end(LexerCtx* ctx, TextSpan start) {
  return (TextSpan){.start = start.start,
                    .len = ctx->pos - start.start,
                    .start_line = start.start_line,
                    .start_col = start.start_col};
}

static bool make_single(LexerCtx* ctx, JsonToken* token, JsonTokenType type) {
  TextSpan span = capture_start(ctx);
  advance(ctx);
  *token = (JsonToken){.type = type, .value = NULL, .span = capture_end(ctx, span)};
  return true;
}

static int read_hex_digits(LexerCtx* ctx) {
  int value = 0;

  for (int i = 0; i < 4; ++i) {
    if (is_end(ctx)) {
      *ctx->err = (JsonErr){
          .msg = strdup("Unexpected end of input in Unicode escape sequence"),
          .span = capture_start(ctx),
      };
      return -1;
    }

    char c = peek(ctx, 0);
    value <<= 4;

    if (c >= '0' && c <= '9') {
      value |= (c - '0');
    } else if (c >= 'a' && c <= 'f') {
      value |= (c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
      value |= (c - 'A' + 10);
    } else {
      *ctx->err = (JsonErr){
          .msg = strdup("Invalid character in Unicode escape sequence"),
          .span = capture_start(ctx),
      };
      return -1;
    }

    advance(ctx);
  }

  return value;
}

static bool read_string(LexerCtx* ctx, JsonToken* token) {
  TextSpan start = capture_start(ctx);
  advance(ctx); // Skip opening quote

  dstr value = dstr_create();

  while (!is_end(ctx)) {
    char c = peek(ctx, 0);

    switch (c) {
      case '\\':
        advance(ctx);

        if (is_end(ctx)) {
          *ctx->err = (JsonErr){
              .msg = strdup("Unexpected end of input in string escape sequence"),
              .span = capture_end(ctx, start),
          };
        }

        char escaped = peek(ctx, 0);

        switch (escaped) {
          case '"':
          case '\\':
          case '/':
            dstr_append_char(&value, escaped);
            advance(ctx);
            break;
          case 'b':
            dstr_append_char(&value, '\b');
            advance(ctx);
            break;
          case 'f':
            dstr_append_char(&value, '\f');
            advance(ctx);
            break;
          case 'n':
            dstr_append_char(&value, '\n');
            advance(ctx);
            break;
          case 'r':
            dstr_append_char(&value, '\r');
            advance(ctx);
            break;
          case 't':
            dstr_append_char(&value, '\t');
            advance(ctx);
            break;
          case 'u': {
            advance(ctx);
            int unicode_val = read_hex_digits(ctx);
            if (unicode_val < 0) {
              *ctx->err = (JsonErr){
                  .msg = strdup("Invalid Unicode escape sequence in string"),
                  .span = capture_end(ctx, start),
              };
              return false;
            }
            dstr_append_char(&value, (char)unicode_val);
            break;
          }
          default: {
            dstr msg = dstr_fmt("Invalid escape character %c in string literal", escaped);
            *ctx->err = (JsonErr){
                .msg = dstr_to_cstr(msg),
                .span = capture_end(ctx, start),
            };
            dstr_destroy(msg);
            return false;
          }
        }

        continue;

      case '\n':
      case '\r':
        *ctx->err = (JsonErr){
            .msg = strdup("Unescaped newline in string literal"),
            .span = capture_end(ctx, start),
        };
        return false;

      case '"':
        advance(ctx); // Skip closing quote
        *token = (JsonToken){
            .type = JSON_TOKEN_STRING,
            .value = dstr_to_cstr(value),
            .span = capture_end(ctx, start),
        };
        dstr_destroy(value);
        return true;

      default:
        dstr_append_char(&value, c);
        advance(ctx);
        break;
    }
  }

  *ctx->err = (JsonErr){
      .msg = strdup("Unexpected end of input in string literal"),
      .span = capture_end(ctx, start),
  };
  dstr_destroy(value);
  return false;
}

static bool read_number(LexerCtx* ctx, JsonToken* token) {
  TextSpan start = capture_start(ctx);
  dstr num_str = dstr_create();

  if (peek(ctx, 0) == '-')
    dstr_append_char(&num_str, '-');

  if (peek(ctx, 0) == '0') {
    dstr_append_char(&num_str, '0');
    advance(ctx);
  } else if (isdigit(peek(ctx, 0))) {
    while (isdigit(peek(ctx, 0))) {
      dstr_append_char(&num_str, peek(ctx, 0));
      advance(ctx);
    }
  } else {
    *ctx->err = (JsonErr){
        .msg = strdup("Invalid number format: expected digit after '-'"),
        .span = capture_end(ctx, start),
    };
    dstr_destroy(num_str);
    return false;
  }

  if (peek(ctx, 0) == '.') {
    dstr_append_char(&num_str, '.');
    advance(ctx);

    if (!isdigit(peek(ctx, 0))) {
      *ctx->err = (JsonErr){
          .msg = strdup("Invalid number format: expected digit after '.'"),
          .span = capture_end(ctx, start),
      };
      dstr_destroy(num_str);
      return false;
    }

    while (isdigit(peek(ctx, 0))) {
      dstr_append_char(&num_str, peek(ctx, 0));
      advance(ctx);
    }
  }

  if (peek(ctx, 0) == 'e' || peek(ctx, 0) == 'E') {
    dstr_append_char(&num_str, peek(ctx, 0));
    advance(ctx);

    if (peek(ctx, 0) == '+' || peek(ctx, 0) == '-') {
      dstr_append_char(&num_str, peek(ctx, 0));
      advance(ctx);
    }

    if (!isdigit(peek(ctx, 0))) {
      *ctx->err = (JsonErr){
          .msg = strdup("Invalid number format: expected digit in exponent"),
          .span = capture_end(ctx, start),
      };
      dstr_destroy(num_str);
      return false;
    }

    while (isdigit(peek(ctx, 0))) {
      dstr_append_char(&num_str, peek(ctx, 0));
      advance(ctx);
    }
  }

  TextSpan span = capture_end(ctx, start);
  char* num_cstr = dstr_to_cstr(num_str);
  char* endptr;
  strtod(num_cstr, &endptr);

  if (*endptr != '\0') {
    *ctx->err = (JsonErr){
        .msg = strdup("Invalid number format: could not parse entire number"),
        .span = span,
    };
    free(num_cstr);
    dstr_destroy(num_str);
    return false;
  }

  *token = (JsonToken){
      .type = JSON_TOKEN_NUMBER,
      .value = num_cstr,
      .span = span,
  };
  dstr_destroy(num_str);
  return true;
}

static bool remaining_starts_with(LexerCtx* ctx, const char* str) {
  strv strv = strv_from(ctx->input + ctx->pos);
  return strv_starts_with(strv, strv_from(str));
}

static bool match_keyword(LexerCtx* ctx, const char* keyword, JsonTokenType type,
                          JsonToken* token) {
  if (remaining_starts_with(ctx, keyword)) {
    TextSpan span = capture_start(ctx);
    for (size_t i = 0; keyword[i] != '\0'; ++i)
      advance(ctx);

    *token = (JsonToken){
        .type = type,
        .value = NULL,
        .span = capture_end(ctx, span),
    };
    return true;
  }

  return false;
}

static bool read_keyword(LexerCtx* ctx, JsonToken* token) {
  if (match_keyword(ctx, "true", JSON_TOKEN_TRUE, token) ||
      match_keyword(ctx, "false", JSON_TOKEN_FALSE, token) ||
      match_keyword(ctx, "null", JSON_TOKEN_NULL, token))
    return true;

  *ctx->err = (JsonErr){
      .msg = strdup("Unexpected token: expected 'true', 'false', or 'null'"),
      .span = capture_start(ctx),
  };
  return false;
}

static bool lexer_next(LexerCtx* ctx, JsonToken* token) {
  skip_trivial_chars(ctx);

  if (is_end(ctx)) {
    *token = (JsonToken){
        .type = JSON_TOKEN_EOF,
        .value = NULL,
        .span =
            (TextSpan){.start = ctx->pos, .len = 0, .start_line = ctx->line, .start_col = ctx->col},
    };
    return true;
  }

  char c = peek(ctx, 0);

  switch (c) {
    case '{':
      return make_single(ctx, token, JSON_TOKEN_LBRACE);
    case '}':
      return make_single(ctx, token, JSON_TOKEN_RBRACE);
    case '[':
      return make_single(ctx, token, JSON_TOKEN_LBRACKET);
    case ']':
      return make_single(ctx, token, JSON_TOKEN_RBRACKET);
    case ':':
      return make_single(ctx, token, JSON_TOKEN_COLON);
    case ',':
      return make_single(ctx, token, JSON_TOKEN_COMMA);
    case '"':
      return read_string(ctx, token);
    case '-':
    case '0' ... '9':
      return read_number(ctx, token);
    default:
      return read_keyword(ctx, token);
  }
}

static bool tokenize(const char* input, Darr* tokens, JsonErr* err) {
  LexerCtx ctx = {
      .input = input,
      .err = err,
      .input_len = strlen(input),
      .pos = 0,
      .col = 1,
      .line = 1,
  };

  while (true) {
    JsonToken token;

    if (!lexer_next(&ctx, &token)) {
      return false;
    }

    darr_push_back(tokens, &token);
    if (token.type == JSON_TOKEN_EOF)
      break;
  }

  return true;
}

void destroy_token(JsonToken* token) {
  free(token->value);
}

static JsonToken* current_token(ParserCtx* ctx) {
  return darr_get(ctx->tokens, ctx->pos);
}

static JsonToken* eat(ParserCtx* ctx, JsonTokenType expected) {
  JsonToken* token = current_token(ctx);
  if (!token || token->type != expected) {
    *ctx->err = (JsonErr){
        .msg = strdup("Unexpected token"),
        .span = token ? token->span : (TextSpan){.start = 0, .len = 0},
    };
    return NULL;
  }
  ctx->pos++;
  return token;
}

TextSpan combine(TextSpan start, TextSpan end) {
  return (TextSpan){.start = MIN(start.start, end.start),
                    .len = ABS(end.start - start.start),
                    .start_line = MIN(start.start, end.start_line),
                    .start_col = MIN(start.start, end.start_col)};
}

void destroy_json_value(JsonValue* value) {
  switch (value->type) {
    case JSON_ARRAY:
      darr_destroy(&value->array_value);
      break;
    case JSON_OBJECT:
      ht_destroy(&value->object_value);
      break;
    case JSON_STRING:
      free(value->string_value);
      break;
    default:
      break;
  }
}

static bool parse_object(ParserCtx* ctx, JsonValue* out, char* id);
static bool parse_array(ParserCtx* ctx, JsonValue* out, char* id);

static bool parse_null(ParserCtx* ctx, JsonValue* out, char* id) {
  JsonToken* token = eat(ctx, JSON_TOKEN_NULL);
  if (!token)
    return false;

  *out = (JsonValue){
      .type = JSON_NULL,
      .id = strdup(id),
      .span = token->span,
  };
  return true;
}

static bool parse_number(ParserCtx* ctx, JsonValue* out, char* id) {
  JsonToken* token = eat(ctx, JSON_TOKEN_NUMBER);
  if (!token)
    return false;

  char* endptr;
  double num = strtod(token->value, &endptr);
  if (*endptr != '\0') {
    *ctx->err = (JsonErr){
        .msg = strdup("Invalid number format: could not parse entire number"),
        .span = token->span,
    };
    return false;
  }

  *out = (JsonValue){
      .type = JSON_NUMBER,
      .id = strdup(id),
      .span = token->span,
      .number_value = num,
  };
  return true;
}

static bool parse_bool(ParserCtx* ctx, JsonValue* out, bool val, char* id) {
  JsonToken* token = eat(ctx, val ? JSON_TOKEN_TRUE : JSON_TOKEN_FALSE);
  if (!token)
    return false;

  *out = (JsonValue){
      .type = JSON_BOOL,
      .id = strdup(id),
      .span = token->span,
      .bool_value = val,
  };
  return true;
}

static bool parse_string(ParserCtx* ctx, JsonValue* out, char* id) {
  JsonToken* token = eat(ctx, JSON_TOKEN_STRING);
  if (!token)
    return false;

  *out = (JsonValue){
      .type = JSON_STRING,
      .id = strdup(id),
      .span = token->span,
      .string_value = strdup(token->value),
  };
  return true;
}

static bool parse_value(ParserCtx* ctx, JsonValue* output, char* id) {
  JsonToken* token = darr_get(ctx->tokens, ctx->pos);

  if (!token) {
    *ctx->err = (JsonErr){
        .msg = strdup("Unexpected end of input while parsing value"),
        .span = (TextSpan){.start = 0, .len = 0, .start_line = 0, .start_col = 0},
    };
    return false;
  }

  switch (token->type) {
    case JSON_TOKEN_LBRACE:
      return parse_object(ctx, output, id);
    case JSON_TOKEN_LBRACKET:
      return parse_array(ctx, output, id);
    case JSON_TOKEN_STRING:
      return parse_string(ctx, output, id);
    case JSON_TOKEN_NUMBER:
      return parse_number(ctx, output, id);
    case JSON_TOKEN_TRUE:
      return parse_bool(ctx, output, true, id);
    case JSON_TOKEN_FALSE:
      return parse_bool(ctx, output, false, id);
    case JSON_TOKEN_NULL:
      return parse_null(ctx, output, id);
    default:
      *ctx->err = (JsonErr){
          .msg = strdup("Unexpected token while parsing value"),
          .span = token->span,
      };
      return false;
  }
}

static bool parse_array(ParserCtx* ctx, JsonValue* out, char* id) {
  JsonToken* start = eat(ctx, JSON_TOKEN_LBRACKET);
  Darr elements = darr_create(sizeof(JsonValue));
  elements.val_destructor = (void (*)(void*))destroy_json_value;

  if (current_token(ctx)->type == JSON_TOKEN_RBRACKET) {
    JsonToken* end = eat(ctx, JSON_TOKEN_RBRACKET);
    *out = (JsonValue){
        .type = JSON_ARRAY,
        .id = strdup(id),
        .span = combine(start->span, end->span),
        .array_value = elements,
    };
    return true;
  }

  size_t idx = 0;
  while (true) {
    dstr item_id = dstr_fmt("%s[%zu]", id, idx);
    JsonValue element;

    if (!parse_value(ctx, &element, item_id)) {
      dstr_destroy(item_id);
      darr_destroy(&elements);
      return false;
    }

    darr_push_back(&elements, &element);

    switch (current_token(ctx)->type) {
      case JSON_TOKEN_COMMA:
        eat(ctx, JSON_TOKEN_COMMA);
        idx++;
        dstr_destroy(item_id);
        continue;
      case JSON_TOKEN_RBRACKET: {
        JsonToken* end = eat(ctx, JSON_TOKEN_RBRACKET);
        *out = (JsonValue){
            .type = JSON_ARRAY,
            .id = strdup(id),
            .span = combine(start->span, end->span),
            .array_value = elements,
        };
        dstr_destroy(item_id);
        return true;
      }
      default:
        *ctx->err = (JsonErr){
            .msg = strdup("Expected ',' or ']' in array literal"),
            .span = current_token(ctx)->span,
        };
        dstr_destroy(item_id);
        darr_destroy(&elements);
        return false;
    }
  }
}

static bool parse_object(ParserCtx* ctx, JsonValue* out, char* id) {
  JsonToken* start = eat(ctx, JSON_TOKEN_LBRACE);
  HashTable props = ht_create(sizeof(JsonValue));
  props.val_destructor = (void (*)(void*))destroy_json_value;

  if (current_token(ctx)->type == JSON_TOKEN_RBRACE) {
    JsonToken* end = eat(ctx, JSON_TOKEN_RBRACE);
    *out = (JsonValue){
        .type = JSON_OBJECT,
        .id = strdup(id),
        .span = combine(start->span, end->span),
        .object_value = props,
    };
    return true;
  }

  while (true) {
    JsonToken* key_token = eat(ctx, JSON_TOKEN_STRING);
    if (!key_token) {
      ht_destroy(&props);
      return false;
    }

    dstr key = dstr_fmt("%s.%s", id, key_token->value);
    eat(ctx, JSON_TOKEN_COLON);
    JsonValue value;

    if (!parse_value(ctx, &value, key)) {
      dstr_destroy(key);
      ht_destroy(&props);
      return false;
    }

    ht_set(&props, key_token->value, &value);

    switch (current_token(ctx)->type) {
      case JSON_TOKEN_COMMA:
        eat(ctx, JSON_TOKEN_COMMA);
        dstr_destroy(key);
        break;
      case JSON_TOKEN_RBRACE: {
        JsonToken* end = eat(ctx, JSON_TOKEN_RBRACE);
        *out = (JsonValue){
            .type = JSON_OBJECT,
            .id = strdup(id),
            .span = combine(start->span, end->span),
            .object_value = props,
        };
        dstr_destroy(key);
        return true;
      }
      default:
        *ctx->err = (JsonErr){
            .msg = strdup("Expected ',' or '}' in object literal"),
            .span = current_token(ctx)->span,
        };
        dstr_destroy(key);
        ht_destroy(&props);
        return false;
    }
  }
}

bool json_free(JsonValue* value) {
  destroy_json_value(value);
  return true;
}

bool json_parse(const char* input, JsonValue* out, char** err_msg) {
  JsonErr err = {0};
  Darr tokens = darr_create(sizeof(JsonToken));
  tokens.val_destructor = (void (*)(void*))destroy_token;

  if (!tokenize(input, &tokens, &err)) {
    dstr msg = dstr_fmt("Error at line %zu, column %zu: %s", err.span.start_line,
                        err.span.start_col, err.msg);
    *err_msg = strdup(msg);
    darr_destroy(&tokens);
    free(err.msg);
    dstr_destroy(msg);
    return false;
  }

  ParserCtx parser_ctx = {
      .tokens = &tokens,
      .pos = 0,
      .err = &err,
  };

  if (!parse_value(&parser_ctx, out, "$")) {
    dstr msg = dstr_fmt("Error at line %zu, column %zu: %s", err.span.start_line,
                        err.span.start_col, err.msg);
    *err_msg = strdup(msg);
    free(err.msg);
    dstr_destroy(msg);
    return false;
  }

  darr_destroy(&tokens);
  return true;
}

bool json_get(JsonValue* root, const char* path, JsonValue** out) {
  Darr segments = strv_split(strv_from(path), '.');

  JsonValue* current = root;
  for (size_t i = 0; i < segments.size; ++i) {
    strv* segment = (strv*)darr_get(&segments, i);

    if (current->type == JSON_OBJECT) {
      JsonValue* next = ht_get(&current->object_value, segment->data);

      if (!next) {
        darr_destroy(&segments);
        return false;
      }
      current = next;
    } else if (current->type == JSON_ARRAY) {
      char* endptr;
      long idx = strtol(segment->data, &endptr, 10);
      if (*endptr != '\0' || idx < 0 || (size_t)idx >= current->array_value.size) {
        darr_destroy(&segments);
        return false;
      }

      JsonValue* next = darr_get(&current->array_value, idx);
      if (!next) {
        darr_destroy(&segments);
        return false;
      }
      current = next;
    } else {
      darr_destroy(&segments);
    }
  }

  *out = current;
  darr_destroy(&segments);
  return true;
}

dstr json_stringify(const JsonValue* value, int indent) {
  dstr result = dstr_create();

  switch (value->type) {
    case JSON_NULL:
      dstr_append(&result, "null");
      break;

    case JSON_BOOL:
      dstr_append(&result, value->bool_value ? "true" : "false");
      break;

    case JSON_NUMBER:
      dstr_append_fmt(&result, "%g", value->number_value);
      break;

    case JSON_STRING:
      dstr_append_char(&result, '"');
      for (size_t i = 0; i < strlen(value->string_value); ++i) {
        char c = value->string_value[i];
        switch (c) {
          case '"':
            dstr_append(&result, "\\\"");
            break;
          case '\\':
            dstr_append(&result, "\\\\");
            break;
          case '\b':
            dstr_append(&result, "\\b");
            break;
          case '\f':
            dstr_append(&result, "\\f");
            break;
          case '\n':
            dstr_append(&result, "\\n");
            break;
          case '\r':
            dstr_append(&result, "\\r");
            break;
          case '\t':
            dstr_append(&result, "\\t");
            break;
          default:
            if ((unsigned char)c < 0x20) {
              dstr_append_fmt(&result, "\\u%04x", (unsigned char)c);
            } else {
              dstr_append_char(&result, c);
            }
        }
      }
      dstr_append_char(&result, '"');
      break;

    case JSON_ARRAY: {
      dstr_append_char(&result, '[');

      if (indent >= 0)
        dstr_append_char(&result, '\n');

      for (size_t i = 0; i < value->array_value.size; ++i) {
        JsonValue* el = darr_get(&value->array_value, i);
        dstr el_str = json_stringify(el, indent >= 0 ? indent + 2 : -1);

        if (indent >= 0)
          dstr_append_fmt(&result, "%*s", indent + 2, "");

        dstr_append(&result, el_str);
        dstr_destroy(el_str);

        if (i < value->array_value.size - 1)
          dstr_append_char(&result, ',');

        if (indent >= 0)
          dstr_append_char(&result, '\n');
      }

      if (indent >= 0)
        dstr_append_fmt(&result, "%*s", indent, "");
      dstr_append_char(&result, ']');

      break;
    }

    case JSON_OBJECT: {
      dstr_append_char(&result, '{');

      if (indent >= 0)
        dstr_append_char(&result, '\n');

      size_t count = 0;
      for (size_t i = 0; i < value->object_value.capacity; ++i) {
        HashTableEntry entry = value->object_value.entries[i];

        if (!entry.key || entry.is_deleted)
          continue;

        dstr el_str = json_stringify((JsonValue*)entry.value, indent >= 0 ? indent + 2 : -1);

        if (indent >= 0)
          dstr_append_fmt(&result, "%*s", indent + 2, "");

        dstr_append_fmt(&result, "\"%s\": %s", entry.key, el_str);
        dstr_destroy(el_str);

        if (count < value->object_value.size - 1)
          dstr_append_char(&result, ',');

        if (indent >= 0)
          dstr_append_char(&result, '\n');

        count++;
      }

      if (indent >= 0)
        dstr_append_fmt(&result, "%*s", indent, "");
      dstr_append_char(&result, '}');
      break;
    }
  }

  return result;
}

#endif // !C_UTILS_JSON_H
