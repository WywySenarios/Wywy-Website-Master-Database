/**
 * Helpers for writing to cursors. Variable names are assumed.
 */

#define decrease_remaining_size(size)                                          \
  ({                                                                           \
    n = size;                                                                  \
    if (remaining_size < n) {                                                  \
      errno = ENOMEM;                                                          \
      goto end;                                                                  \
    }                                                                          \
    remaining_size -= n;                                                       \
  })

#define cur_memcpy(str)                                                        \
  ({                                                                           \
    decrease_remaining_size(strlen(str));                                      \
    memcpy(cur, str, n);                                                       \
    cur += n;                                                                  \
  })

#define cur_write_table_name()                                                 \
  ({                                                                           \
    decrease_remaining_size(table_name_len);                                   \
    memcpy(cur, options->table_name, table_name_len);                          \
    to_lower_snake_case_n(cur, table_name_len);                                \
    cur += table_name_len;                                                     \
  })

#define cur_write_column_name(column_name)                                     \
  ({                                                                           \
    decrease_remaining_size(strlen(column_name));                              \
    memcpy(cur, column_name, n);                                               \
    to_lower_snake_case_n(cur, n);                                             \
    cur += n;                                                                  \
  })

#define cur_write_full_column_name()                                           \
  ({                                                                           \
    decrease_remaining_size(table_name_len + 1);                               \
    memcpy(cur, options->table_name, table_name_len);                          \
    to_lower_snake_case_n(cur, table_name_len);                                \
    cur += table_name_len;                                                     \
    *cur++ = '.';                                                              \
    decrease_remaining_size(strlen(options->schema[i].name));                  \
    memcpy(cur, options->schema[i].name, n);                                   \
    to_lower_snake_case_n(cur, n);                                             \
    cur += n;                                                                  \
  })

// @TODO optimize cur_append
#define cur_append(character)                                                  \
  ({                                                                           \
    decrease_remaining_size(1);                                                \
    *cur++ = character;                                                        \
  })