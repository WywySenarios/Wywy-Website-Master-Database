#include <stdlib.h>

extern int strlen_safe(char *s);
extern void to_snake_case(char *src);
/**
 * Attempts to convert a given string to lower snake case.
 * @param src The string to convert. This function does not free src, but it
 * does modify it in place.
 */
extern void to_lower_snake_case(char *src);
/**
 * Attempts to convert a given string to lower snake case.
 * @param src The string to convert. This function does not free src, but it
 * does modify it in place.
 * @param size The size of the string to convert.
 */
extern void to_lower_snake_case_n(char *src, const size_t size);
extern int str_cci_cmp(char const *a, char const *b);
/**
 * Finds the string length of an integer. e.g. 0 -> 1, 999 -> 3.
 * @param integer The target integer.
 */
extern int int_str_len(int integer);