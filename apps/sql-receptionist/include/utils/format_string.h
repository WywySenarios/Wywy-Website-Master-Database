extern int strlen_safe(char *s);
extern void to_snake_case(char *src);
extern void to_lower_snake_case(char *src);
extern int str_cci_cmp(char const *a, char const *b);
/**
 * Finds the string length of an integer. e.g. 0 -> 1, 999 -> 3.
 * @param integer The target integer.
 */
extern int int_str_len(int integer);