#include <stddef.h>
int constant_time_string_equality(const char *s1, const char *s2, size_t n) {
  int diff = 0;
  for (int i = 0; i < n; i++) {
    diff |= (unsigned char)s1[i] ^ (unsigned char)s2[i];
  }

  return diff == 0;
}