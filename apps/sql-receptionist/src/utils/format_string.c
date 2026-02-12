#include <ctype.h>
#include <string.h>

/**
 * @param s The string to check.
 * @returns The string length of s if it exists, and 0 otherwise.
 */
int strlen_safe(char *s) { return s == NULL ? 0 : strlen(s); }

/**
 * Attempts to convert a given string to snake case.
 * @param src The string to convert. This function does not free src, but it
 * does modify it in place.
 */
void to_snake_case(char *src) {
  size_t src_len = strlen(src);
  for (unsigned i = 0; i < src_len; i++) {
    switch (src[i]) {
    case ' ':
    case '-':
    case '.':
      src[i] = '_';
      break;
    }
  }
}

/**
 * Attempts to convert a given string to snake case.
 * @param src The string to convert. This function does not free src, but it
 * does modify it in place.
 */
void to_lower_snake_case(char *src) {
  size_t src_len = strlen(src);
  for (unsigned i = 0; i < src_len; i++) {
    switch (src[i]) {
    case ' ':
    case '-':
    case '.':
      src[i] = '_';
      break;
    default:
      src[i] = tolower((unsigned char)src[i]); // @WARNING why unsigned?
      break;
    }
  }
}

int str_cci_cmp(const char *a, const char *b) {
  while (*a && *b) {
    // Skip underscores and spaces in both strings
    while (*a == '_' || *a == ' ')
      a++;
    while (*b == '_' || *b == ' ')
      b++;

    // If we reach the end after skipping, break
    if (!*a || !*b)
      break;

    char ca = tolower((unsigned char)*a);
    char cb = tolower((unsigned char)*b);

    if (ca != cb)
      return (unsigned char)ca - (unsigned char)cb;

    a++;
    b++;
  }

  // Skip any remaining underscores/spaces at the end
  while (*a == '_' || *a == ' ')
    a++;
  while (*b == '_' || *b == ' ')
    b++;

  return (unsigned char)tolower((unsigned char)*a) -
         (unsigned char)tolower((unsigned char)*b);
}