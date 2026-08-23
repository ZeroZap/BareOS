#include "xy_string.h"

void *xy_memset(void *dst, uint8_t val, uint32_t len) {
  uint8_t *out;
  uint32_t i;

  if (dst == NULL) {
    return NULL;
  }
  out = (uint8_t *)dst;
  for (i = 0U; i < len; i++) {
    out[i] = val;
  }
  return dst;
}

void *xy_memcpy(void *dst, const void *src, uint32_t len) {
  uint8_t *out;
  const uint8_t *in;
  uint32_t i;

  if (dst == NULL || src == NULL) {
    return NULL;
  }
  out = (uint8_t *)dst;
  in = (const uint8_t *)src;
  for (i = 0U; i < len; i++) {
    out[i] = in[i];
  }
  return dst;
}

int32_t xy_memcmp(const void *lhs, const void *rhs, uint32_t len) {
  const uint8_t *a;
  const uint8_t *b;
  uint32_t i;

  if (lhs == NULL || rhs == NULL) {
    return -1;
  }
  a = (const uint8_t *)lhs;
  b = (const uint8_t *)rhs;
  for (i = 0U; i < len; i++) {
    if (a[i] != b[i]) {
      return (int32_t)a[i] - (int32_t)b[i];
    }
  }
  return 0;
}
