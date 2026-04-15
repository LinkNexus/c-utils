#ifndef C_UTILS_JSON_H
#define C_UTILS_JSON_H

#include "darr.h"
#include "dstr.h"
#include "ht.h"
#include "strv.h"
#include "utils.h"
#include <complex.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef enum {
  JSON_NULL,
  JSON_BOOL,
  JSON_NUMBER,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT
} json_type;

typedef enum {
  TOKEN_STRING, // 0
  TOKEN_NUMBER,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_NULL,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_COLON,
  TOKEN_COMMA,
  TOKEN_EOF,
} json_token_type;

typedef struct {
  size_t start;
  size_t len;
  size_t start_line;
  size_t start_col;
} text_span;

typedef struct {
  char *msg;
  text_span span;
} json_error;

typedef struct {
  json_type type;
  char *id;
  text_span span;
  union {
    bool bool_value;
    double number_value;
    char *string_value;
    darr array_value;
    ht object_value;
  };
} json_value;

typedef struct {
  const char *input;
  json_error *err;
  size_t input_len;
  size_t pos;
  size_t col;
  size_t line;
} lexer_ctx;

typedef struct {
  json_token_type type;
  char *value;
  text_span span;
} json_token;

typedef struct {
  darr *tokens;
  size_t pos;
  json_error *err;
} parser_ctx;

static bool is_end(lexer_ctx *ctx) { return ctx->pos >= ctx->input_len; }

static char peek(lexer_ctx *ctx, size_t offset) {
  size_t idx = ctx->pos + offset;
  if (idx >= ctx->input_len)
    return '\0';
  return ctx->input[idx];
}

static void advance(lexer_ctx *ctx) {
  if (is_end(ctx))
    return;

  ctx->pos++;
  ctx->col++;
}

static void advance_line(lexer_ctx *ctx) {
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

static void skip_trivial_chars(lexer_ctx *ctx) {
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

static text_span capture_start(lexer_ctx *ctx) {
  return (text_span){.start = ctx->pos,
                     .len = 0,
                     .start_line = ctx->line,
                     .start_col = ctx->col};
}

static text_span capture_end(lexer_ctx *ctx, text_span start) {
  return (text_span){.start = start.start,
                     .len = ctx->pos - start.start,
                     .start_line = start.start_line,
                     .start_col = start.start_col};
}

static bool make_single(lexer_ctx *ctx, json_token *token,
                        json_token_type type) {
  text_span span = capture_start(ctx);
  advance(ctx);
  *token =
      (json_token){.type = type, .value = NULL, .span = capture_end(ctx, span)};
  return true;
}

static int read_hex_digits(lexer_ctx *ctx) {
  int value = 0;

  for (int i = 0; i < 4; ++i) {
    if (is_end(ctx)) {
      *ctx->err = (json_error){
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
      *ctx->err = (json_error){
          .msg = strdup("Invalid character in Unicode escape sequence"),
          .span = capture_start(ctx),
      };
      return -1;
    }

    advance(ctx);
  }

  return value;
}

static bool read_string(lexer_ctx *ctx, json_token *token) {
  text_span start = capture_start(ctx);
  advance(ctx); // Skip opening quote

  dstr value = dstr_create();

  while (!is_end(ctx)) {
    char c = peek(ctx, 0);

    switch (c) {
    case '\\':
      advance(ctx);

      if (is_end(ctx)) {
        *ctx->err = (json_error){
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
        break;
      case 'b':
        dstr_append_char(&value, '\b');
        break;
      case 'f':
        dstr_append_char(&value, '\f');
        break;
      case 'n':
        dstr_append_char(&value, '\n');
        break;
      case 'r':
        dstr_append_char(&value, '\r');
        break;
      case 't':
        dstr_append_char(&value, '\t');
        break;
      case 'u': {
        advance(ctx);
        int unicode_val = read_hex_digits(ctx);
        if (unicode_val < 0) {
          *ctx->err = (json_error){
              .msg = strdup("Invalid Unicode escape sequence in string"),
              .span = capture_end(ctx, start),
          };
          return false;
        }
        dstr_append_char(&value, (char)unicode_val);
        break;
      }
      default: {
        dstr msg =
            dstr_fmt("Invalid escape character %c in string literal", escaped);
        *ctx->err = (json_error){
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
      *ctx->err = (json_error){
          .msg = strdup("Unescaped newline in string literal"),
          .span = capture_end(ctx, start),
      };
      return false;

    case '"':
      advance(ctx); // Skip closing quote
      *token = (json_token){
          .type = TOKEN_STRING,
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

  *ctx->err = (json_error){
      .msg = strdup("Unexpected end of input in string literal"),
      .span = capture_end(ctx, start),
  };
  dstr_destroy(value);
  return false;
}

static bool read_number(lexer_ctx *ctx, json_token *token) {
  text_span start = capture_start(ctx);
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
    *ctx->err = (json_error){
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
      *ctx->err = (json_error){
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
      *ctx->err = (json_error){
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

  text_span span = capture_end(ctx, start);
  char *num_cstr = dstr_to_cstr(num_str);
  char *endptr;
  strtod(num_cstr, &endptr);

  if (*endptr != '\0') {
    *ctx->err = (json_error){
        .msg = strdup("Invalid number format: could not parse entire number"),
        .span = span,
    };
    free(num_cstr);
    dstr_destroy(num_str);
    return false;
  }

  *token = (json_token){
      .type = TOKEN_NUMBER,
      .value = num_cstr,
      .span = span,
  };
  dstr_destroy(num_str);
  return true;
}

static bool remaining_starts_with(lexer_ctx *ctx, const char *str) {
  strv strv = strv_from(ctx->input + ctx->pos);
  return strv_starts_with(strv, strv_from(str));
}

static bool match_keyword(lexer_ctx *ctx, const char *keyword,
                          json_token_type type, json_token *token) {
  if (remaining_starts_with(ctx, keyword)) {
    text_span span = capture_start(ctx);
    for (size_t i = 0; keyword[i] != '\0'; ++i)
      advance(ctx);

    *token = (json_token){
        .type = type,
        .value = NULL,
        .span = capture_end(ctx, span),
    };
    return true;
  }

  return false;
}

static bool read_keyword(lexer_ctx *ctx, json_token *token) {
  if (match_keyword(ctx, "true", TOKEN_TRUE, token) ||
      match_keyword(ctx, "false", TOKEN_FALSE, token) ||
      match_keyword(ctx, "null", TOKEN_NULL, token))
    return true;

  *ctx->err = (json_error){
      .msg = strdup("Unexpected token: expected 'true', 'false', or 'null'"),
      .span = capture_start(ctx),
  };
  return false;
}

static bool lexer_next(lexer_ctx *ctx, json_token *token) {
  skip_trivial_chars(ctx);

  if (is_end(ctx)) {
    *token = (json_token){
        .type = TOKEN_EOF,
        .value = NULL,
        .span = (text_span){.start = ctx->pos,
                            .len = 0,
                            .start_line = ctx->line,
                            .start_col = ctx->col},
    };
    return true;
  }

  char c = peek(ctx, 0);

  switch (c) {
  case '{':
    return make_single(ctx, token, TOKEN_LBRACE);
  case '}':
    return make_single(ctx, token, TOKEN_RBRACE);
  case '[':
    return make_single(ctx, token, TOKEN_LBRACKET);
  case ']':
    return make_single(ctx, token, TOKEN_RBRACKET);
  case ':':
    return make_single(ctx, token, TOKEN_COLON);
  case ',':
    return make_single(ctx, token, TOKEN_COMMA);
  case '"':
    return read_string(ctx, token);
  case '-':
  case '0' ... '9':
    return read_number(ctx, token);
  default:
    return read_keyword(ctx, token);
  }
}

static bool tokenize(const char *input, darr *tokens, json_error *err) {
  lexer_ctx ctx = {
      .input = input,
      .err = err,
      .input_len = strlen(input),
      .pos = 0,
      .col = 1,
      .line = 1,
  };

  while (true) {
    json_token *token = xmalloc(sizeof *token);

    if (!lexer_next(&ctx, token)) {
      free(token);
      return false;
    }

    darr_push_back(tokens, token);
    if (token->type == TOKEN_EOF)
      break;
  }

  return true;
}

void destroy_token(json_token *token) {
  if (token->value)
    free(token->value);
  free(token);
}

static json_token *current_token(parser_ctx *ctx) {
  return darr_get(ctx->tokens, ctx->pos);
}

static json_token *eat(parser_ctx *ctx, json_token_type expected) {
  json_token *token = current_token(ctx);
  if (!token || token->type != expected) {
    *ctx->err = (json_error){
        .msg = strdup("Unexpected token"),
        .span = token ? token->span : (text_span){.start = 0, .len = 0},
    };
    return NULL;
  }
  ctx->pos++;
  return token;
}

text_span combine(text_span start, text_span end) {
  return (text_span){.start = MIN(start.start, end.start),
                     .len = ABS(end.start - start.start),
                     .start_line = MIN(start.start, end.start_line),
                     .start_col = MIN(start.start, end.start_col)};
}

void destroy_json_value(json_value *value) {
  if (!value)
    return;

  switch (value->type) {
  case JSON_STRING:
    free(value->string_value);
    break;
  case JSON_ARRAY:
    darr_destroy(&value->array_value);
    break;
  case JSON_OBJECT:
    ht_iter iter = ht_iter_create(&value->object_value);
    while (ht_iter_next(&iter)) {
      json_value *prop_val = iter.value;
      destroy_json_value(prop_val);
    }
    ht_destroy(&value->object_value);
    break;
  default:
    break;
  }

  free(value->id);
  free(value);
}

static bool parse_object(parser_ctx *ctx, json_value *output, char *id);
static bool parse_array(parser_ctx *ctx, json_value *output, char *id);

static bool parse_value(parser_ctx *ctx, json_value *output, char *id) {
  json_token *token = darr_get(ctx->tokens, ctx->pos);

  if (!token) {
    *ctx->err = (json_error){
        .msg = strdup("Unexpected end of input while parsing value"),
        .span =
            (text_span){.start = 0, .len = 0, .start_line = 0, .start_col = 0},
    };
    return false;
  }

  switch (token->type) {
  case TOKEN_LBRACE:
    return parse_object(ctx, output, id);
  case TOKEN_LBRACKET:
    return parse_array(ctx, output);
  case TOKEN_STRING:
    return parse_string(ctx, output);
  case TOKEN_NUMBER:
    return parse_number(ctx, output);
  case TOKEN_TRUE:
  case TOKEN_FALSE:
    return parse_bool(ctx, output);
  case TOKEN_NULL:
    return parse_null(ctx, output);
  default:
    *ctx->err = (json_error){
        .msg = strdup("Unexpected token while parsing value"),
        .span = token->span,
    };
    return false;
  }
}

static bool parse_array(parser_ctx *ctx, json_value *output, char *id) {
  json_token *start = eat(ctx, TOKEN_LBRACKET);
  darr items = darr_create(sizeof(json_value));

  if (current_token(ctx)->type == TOKEN_RBRACKET) {
    json_token *end = eat(ctx, TOKEN_RBRACKET);
    *output = (json_value){
        .type = JSON_ARRAY,
        .id = id ? strdup(id) : NULL,
        .span = combine(start->span, end->span),
        .array_value = items,
    };
    return true;
  }

  size_t index = 0;
  while (true) {
    char *item_id = dstr_fmt("%s[%zu]", id, index);
    json_value *item = xmalloc(sizeof(json_value));
    if (!parse_value(ctx, item, item_id)) {
      darr_destroy(&items);
      free(item_id);
      return false;
    }
  }
}

static bool parse_object(parser_ctx *ctx, json_value *output, char *id) {
  json_token *start = eat(ctx, TOKEN_LBRACE);
  ht props = ht_create(sizeof(json_value));

  if (current_token(ctx)->type == TOKEN_RBRACE) {
    json_token *end = eat(ctx, TOKEN_RBRACE);
    *output = (json_value){
        .type = JSON_OBJECT,
        .id = id ? strdup(id) : NULL,
        .span = combine(start->span, end->span),
        .object_value = props,
    };
    return true;
  }

  while (true) {
    json_token *key_token = eat(ctx, TOKEN_STRING);

    if (!key_token) {
      ht_destroy(&props);
      return false;
    }

    dstr key = dstr_fmt("%s.%s", id, key_token->value);
    eat(ctx, TOKEN_COLON);

    json_value *value = xmalloc(sizeof(json_value));
    if (!parse_value(ctx, value)) {
      ht_destroy(&props);
      return false;
    }
    ht_set(&props, key_token->value, &value);

    switch (current_token(ctx)->type) {
    case TOKEN_COMMA:
      eat(ctx, TOKEN_COMMA);
      break;

    case TOKEN_RBRACE: {
      json_token *end = eat(ctx, TOKEN_RBRACE);
      *output = (json_value){
          .type = JSON_OBJECT,
          .id = strdup(id),
          .span = combine(start->span, end->span),
          .object_value = props,
      };
      return true;
    }

    default:
      *ctx->err = (json_error){
          .msg = strdup("Expected ',' or '}' after object property"),
          .span = current_token(ctx)->span,
      };
      ht_destroy(&props);
      return false;
    }
  }
}

static bool json_parse(const char *input, json_value *output, char **err_msg) {
  json_error err = {0};
  darr tokens = darr_create(sizeof(json_token));
  tokens.val_destructor = (void (*)(void *))destroy_token;

  if (!tokenize(input, &tokens, &err)) {
    dstr msg = dstr_fmt("Error at line %zu, column %zu: %s",
                        err.span.start_line, err.span.start_col, err.msg);
    *err_msg = strdup(msg);
    darr_destroy(&tokens);
    free(err.msg);
    dstr_destroy(msg);
    return false;
  }

  darr_destroy(&tokens);
  return true;
}

#endif // !C_UTILS_JSON_H
