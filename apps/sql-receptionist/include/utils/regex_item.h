#include <regex.h>

/**
 * Attempts to check if a match was successfully made. Useful for validation.
 * on failure.
 * @param pattern the regex pattern to run. This string is not
 * freed. This string should not be NULL.
 * @param num_matches the upper bound of number of catpuring groups you expect
 * to get. This returns null if num_matches <= 0.
 * @param cflags for regcomp.
 * @param eflags for regexec.
 * @param target the target string to query.
 * @return -1 if unsuccessful, 0 if there was no match, 1 if there was a match.
 */
extern int regex_check(char *pattern, int num_matches, int cflags, int eflags,
                        const char *target);

/**
 * Attempts to query the given target with the given regex pattern. Returns NULL
 * on failure.
 * @param pattern the regex pattern to run. This string is not freed. This string must not be NULL.
 * @param num_matches the upper bound of number of catpuring groups you expect
 * to get. This returns null if num_matches <= 0.
 * @param cflags for regcomp.
 * @param eflags for regexec.
 * @param target the target string to query.
 * @return the pointer to the regex matches
 */
extern regmatch_t *query_regex(char *pattern, int num_matches, int cflags, int eflags,
                        char *target);
/**
 * Checks if there is a match for the given group number.
 * @param matches the matches to check.
 * @param group_num the group to check for a match.
 * @return true if there is a valid match.
 */
extern int regmatch_has_match(regmatch_t *matches, int group_num);
/**
 * Gets the ith match number (0 -> entire string that was regex'd). May behave
 * unexpectedly if match_num is invalid.
 * @param matches the related matches.
 * @param match_num the match that will be extracted.
 * @param target the string that the regex ran on.
 */
extern char *regmatch_get_match(regmatch_t *matches, int match_num, char *target);