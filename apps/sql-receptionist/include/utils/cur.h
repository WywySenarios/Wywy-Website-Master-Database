/**
 * Helpers for writing to cursors. Variable names are assumed.
 */

#define decrease_remaining_size(remaining_size, size)                          \
  ({                                                                           \
    n = size;                                                                  \
    if (remaining_size < n) {                                                  \
      errno = ENOMEM;                                                          \
      goto end;                                                                \
    }                                                                          \
    remaining_size -= n;                                                       \
  })

#define cur_memcpy(cur, remaining_size, str)                                   \
  ({                                                                           \
    decrease_remaining_size(remaining_size, strlen(str));                      \
    memcpy(cur, str, n);                                                       \
    cur += n;                                                                  \
  })

#define cur_write_table_name(cur, remaining_size)                              \
  ({                                                                           \
    decrease_remaining_size(remaining_size, table_name_len);                   \
    memcpy(cur, options->table_name, table_name_len);                          \
    to_lower_snake_case_n(cur, table_name_len);                                \
    cur += table_name_len;                                                     \
  })

#define cur_write_column_name(cur, remaining_size, column_name)                \
  ({                                                                           \
    decrease_remaining_size(remaining_size, strlen(column_name));              \
    memcpy(cur, column_name, n);                                               \
    to_lower_snake_case_n(cur, n);                                             \
    cur += n;                                                                  \
  })

#define cur_write_full_column_name(cur, remaining_size)                        \
  ({                                                                           \
    decrease_remaining_size(remaining_size, table_name_len + 1);               \
    memcpy(cur, options->table_name, table_name_len);                          \
    to_lower_snake_case_n(cur, table_name_len);                                \
    cur += table_name_len;                                                     \
    *cur++ = '.';                                                              \
    decrease_remaining_size(remaining_size, strlen(options->schema[i].name));  \
    memcpy(cur, options->schema[i].name, n);                                   \
    to_lower_snake_case_n(cur, n);                                             \
    cur += n;                                                                  \
  })

// @TODO optimize cur_append
#define cur_append(cur, remaining_size, character)                             \
  ({                                                                           \
    decrease_remaining_size(remaining_size, 1);                                \
    *cur++ = character;                                                        \
  })

#define cur_shave(cur, remaining_size, length)                                 \
  do {                                                                         \
    cur -= length;                                                             \
    remaining_size += length;                                                  \
  } while (0)

#define revert_cur_checkpoint(cur, remaining_size, cur_checkpoint)             \
  do {                                                                         \
    remaining_size += cur - cur_checkpoint;                                    \
    cur = cur_checkpoint;                                                      \
  } while (0)
