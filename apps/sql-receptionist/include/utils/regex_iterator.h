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
extern void free_regex_iterator(struct regex_iterator *iter);