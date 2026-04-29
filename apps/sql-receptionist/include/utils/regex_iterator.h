#include <regex.h>

struct regex_iterator {
  regex_t *preg;
  regmatch_t *matches;
  int nmatch;
  char *target;
  char *cur;
};

extern struct regex_iterator *
create_regex_iterator(char *pattern, int num_matches, int cflags);
extern void regex_iterator_load_target(struct regex_iterator *iter,
                                       char *new_target);
extern int regex_iterator_match(struct regex_iterator *iter, int eflags);
extern void regex_iterator_advance_cur(struct regex_iterator *iter);
extern int has_match(struct regex_iterator *iter, int group_num);
extern char *regex_iterator_get_match(struct regex_iterator *iter,
                                      int match_num);
/**
 * Attempts to write the value of the ith match number to the buffer. May behave
 * unexpectedly if match_num is invalid. May fail if the iterator never tried to
 * match or if the iterator is invalid.
 * @param iter the related regex iterator. This must not be NULL.
 * @param match_num the match that will be extracted.
 * @returns The size of the text written.
 */
extern size_t regex_iterator_write_match(struct regex_iterator *iter,
                                         int match_num, char *buffer,
                                         size_t buffer_size);
extern void free_regex_iterator(struct regex_iterator *iter);