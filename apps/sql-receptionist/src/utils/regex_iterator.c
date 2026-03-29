/**
 * Helper library for running regex and storing output into strings. Iterator
 * style. Does not have a has_next() function.
 */

#include "utils/regex_iterator.h"
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Creates a new regex iterator based on the given pattern. Returns NULL on
 * failure.
 * @param pattern the pattern for the new regex iterator. This string is not
 * freed. This string should not be NULL.
 * @param num_matches the upper bound of number of catpuring groups you expect
 * to get. This returns null if num_matches <= 0.
 * @param cflags for regcomp.
 * @return the pointer to the new regex iterator.
 */
struct regex_iterator *create_regex_iterator(char *pattern, int num_matches,
                                             int cflags) {
  if (num_matches <= 0)
    return NULL;

  struct regex_iterator *output = malloc(sizeof(struct regex_iterator));

  output->preg = malloc(sizeof(regex_t));

  output->nmatch = num_matches + 1;
  output->matches = malloc(sizeof(regmatch_t) * (num_matches + 1));
  if (regcomp(output->preg, pattern, cflags) != 0) {
    free(output->matches);
    free(output);
    return NULL;
  }

  return output;
}

/**
 * Changes the target string of the given regex_iterator. If there was an old
 * target, that string is not freed.
 * @param iter the regex_iterator to modify. This must not be NULL.
 * @param new_target the new target string. This should not be NULL.
 */
void regex_iterator_load_target(struct regex_iterator *iter, char *new_target) {
  iter->target = new_target;
  iter->cur = new_target;
}

/**
 * Attempts to query the given target. Returns REG_NOMATCH when there is no
 * target. May fail if the regex_iterator is not valid. This does not modify the
 * regex_iterator's target string.
 * @param iter the regex_iterator to run the query on. This must not be NULL.
 * @param eflags the eflags to pass into regexec.
 * @return the result of regexec.
 */
int regex_iterator_match(struct regex_iterator *iter, int eflags) {
  if (!iter->cur)
    return REG_NOMATCH;

  return regexec(iter->preg, iter->cur, iter->nmatch, iter->matches, eflags);
}

/**
 * Attempts to advance the target regex_iterator's cursor.
 * @param iter the target regex_iterator.
 */
void regex_iterator_advance_cur(struct regex_iterator *iter) {
  if (!iter->cur)
    return;

  if (has_match(iter, 0))
    iter->cur += iter->matches[0].rm_eo;
}

/**
 * Checks if there is a match for the given group number.
 * @param iter the related regex iterator. This must not be NULL.
 * @param group_num the group to check for a match.
 * @return true if there is a valid match.
 */
int has_match(struct regex_iterator *iter, int group_num) {
  if (iter->nmatch <= group_num || iter->nmatch <= 0)
    return 0;
  if (iter->matches == NULL || iter->matches[0].rm_eo == -1 ||
      iter->matches[0].rm_so == -1 || iter->matches[group_num].rm_eo == -1 ||
      iter->matches[group_num].rm_so == -1)
    return 0;
  return 1;
}

/**
 * Gets the ith match number (0 -> entire string that was regex'd). May behave
 * unexpectedly if match_num is invalid. May fail if the iterator never tried to
 * match or if the iterator is invalid. Returns NULL if memory allocation fails.
 * @param iter the related regex iterator. This must not be NULL.
 * @param match_num the match that will be extracted.
 */
char *regex_iterator_get_match(struct regex_iterator *iter, int match_num) {
  if (!(0 <= match_num && match_num < iter->nmatch))
    return NULL;

  int output_len =
      iter->matches[match_num].rm_eo - iter->matches[match_num].rm_so;
  char *output = malloc(output_len + 1);
  if (!output)
    return NULL;

  memcpy(output, iter->cur + iter->matches[match_num].rm_so, output_len);
  output[output_len] = '\0';

  return output;
}

/**
 * Frees all pointers associated with a given regex iterator. Does not free the
 * target. Also frees the pointer to the iterator itself.
 * @param iter the regex_iterator to free.
 */
void free_regex_iterator(struct regex_iterator *iter) {
  if (!iter)
    return;

  if (iter->preg) {
    regfree(iter->preg);
    free(iter->preg);
  }
  free(iter->matches);
  free(iter);
}