#ifndef C_UTILS_MATH_H
#define C_UTILS_MATH_H

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  float x;
  float y;
} Vec2;

bool vec2_equal(Vec2 a, Vec2 b) {
  return a.x == b.x && a.y == b.y;
}

Vec2 vec2_add(Vec2 a, Vec2 b) {
  return (Vec2){.x = a.x + b.x, .y = a.y + b.y};
}

Vec2 vec2_sub(Vec2 a, Vec2 b) {
  return (Vec2){.x = a.x - b.x, .y = a.y - b.y};
}

Vec2 vec2_scale(Vec2 v, float s) {
  return (Vec2){.x = v.x * s, .y = v.y * s};
}

float vec2_dot(Vec2 a, Vec2 b) {
  return a.x * b.x + a.y * b.y;
}

float vec2_len_squared(Vec2 v) {
  return v.x * v.x + v.y * v.y;
}

float vec2_len(Vec2 v) {
  return sqrtf(vec2_len_squared(v));
}

Vec2 vec2_normalize(Vec2 v) {
  float len = vec2_len(v);
  if (len == 0) {
    return (Vec2){.x = 0, .y = 0};
  }
  return (Vec2){.x = v.x / len, .y = v.y / len};
}

float vec2_distance(Vec2 a, Vec2 b) {
  return vec2_len(vec2_sub(a, b));
}

float vec2_angle(Vec2 a, Vec2 b) {
  float dot = vec2_dot(a, b);
  float lenA = vec2_len(a);
  float lenB = vec2_len(b);
  if (lenA == 0 || lenB == 0) {
    return 0;
  }
  return acosf(dot / (lenA * lenB));
}

Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) {
  return (Vec2){.x = a.x + t * (b.x - a.x), .y = a.y + t * (b.y - a.y)};
}

Vec2 vec2_tangent(Vec2 v) {
  return (Vec2){.x = -v.y, .y = v.x};
}

int rnd_int(int min, int max) {
  return min + rand() % (max - min + 1);
}

float rnd_float() {
  return (float)rand() / (float)RAND_MAX;
}

float rnd_float_range(float min, float max) {
  return min + rnd_float() * (max - min);
}

#endif // C_UTILS_MATH_H
