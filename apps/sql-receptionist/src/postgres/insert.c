#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#ifndef CUR_LIB
#define CUR_LIB
#include "utils/cur.h"
#endif
#include "postgres.h"
#include "postgres/insert.h"
#include "postgres/schema.h"
#include "utils/format_string.h"
#include "utils/json/json_conversion.h"
#include "utils/regex_item.h"
#include <errno.h>
#include <jansson.h>
#include <libpq-fe.h>
#include <regex.h>
#include <stdbool.h>
#include <string.h>

#define MAX_INSERT_QUERY_SIZE 65536
#define MAX_COLUMN_NAME_SIZE 100
#define LARGEST_COLUMN_NAME_SUFFIX strlen("_altitude_accuracy")

// does NOT validate datatypes
#define write_and_access_column(column_name)                                   \
  ({                                                                           \
    current_column_name = cur;                                                 \
    cur_write_column_name(column_name);                                        \
    current_item = json_object_getn(entry, current_column_name,                \
                                    cur - current_column_name);                \
    cur_append(',');                                                           \
  })
#define write_and_access_child_column_name(column_name, column_suffix)         \
  ({                                                                           \
    current_column_name = cur;                                                 \
    cur_write_column_name(column_name);                                        \
    cur_memcpy(column_suffix);                                                 \
    current_item = json_object_getn(entry, current_column_name,                \
                                    cur - current_column_name);                \
    cur_append(',');                                                           \
  })
#define cur_write_json_value(value)                                            \
  ({                                                                           \
    n = write_json_value(value, cur, remaining_size);                          \
    if (remaining_size <= n) {                                                 \
      errno = ENOMEM;                                                          \
      goto end;                                                                \
    }                                                                          \
    remaining_size -= n;                                                       \
    cur += n;                                                                  \
  })

#define write_next_json_value()                                                \
  ({                                                                           \
    temp_cur = current_column_name;                                            \
    while (*temp_cur != ',' && *temp_cur != ')')                               \
      temp_cur++;                                                              \
    current_item = json_object_getn(entry, current_column_name,                \
                                    temp_cur - current_column_name);           \
    current_column_name = temp_cur + 1;                                        \
    if (current_item) {                                                        \
      cur_write_json_value(current_item);                                      \
      cur_append(',');                                                         \
    } else                                                                     \
      cur_memcpy("NULL,");                                                     \
  })

int validate_and_insert_into(struct insert_options *options, json_t *entry,
                             PGresult **res, PGconn *conn, char *error_buffer) {
  // @TODO use placeholders
  int status = 0; // innocent until proven guilty
  const char *key = NULL;
  const json_t *value = NULL;
  char query[MAX_INSERT_QUERY_SIZE];
  char *current_column_name = NULL;
  char *cur = query;
  char *column_names = NULL;
  size_t remaining_size = MAX_INSERT_QUERY_SIZE;
  size_t n;
  *error_buffer = '\0';

  // write in the beginning of the query
  // we can guarentee this fits into the query buffer because it's the first
  // text to be inserted and is a known constant.
  n = snprintf(cur, remaining_size, "INSERT INTO %s (", options->table_name);
  if (errno) {
    goto end;
  }
  if (remaining_size < n) {
    errno = ENOMEM;
    goto end;
  }
  remaining_size -= n;
  cur += n;

  // check for conformance & write in column names
  size_t columns_consumed = 0;
  json_t *current_item = NULL;
  column_names = cur;
  // check for primary_tag
  if (options->primary_tag) {
    write_and_access_column("primary_tag");

    if (!current_item) {
      memcpy(error_buffer, "Missing column \"primary tag\".",
             strlen("Missing \"primary tag\".") + 1);
      return 0;
    }
    if (!json_is_integer(current_item)) {
      memcpy(
          error_buffer, "Datatype datatype: primary_tag should be an integer.",
          strlen("Datatype mismatch: primary_tag should be an integer.") + 1);
      return 0;
    }
    columns_consumed++;
  }

  for (int i = 0; i < options->schema_count; i++) {
    // @TODO add optionality
    write_and_access_column(options->schema[i].name);
    if (!current_item) {
      snprintf(error_buffer, ERROR_BUFFER_SIZE, "Missing column \"%s\".",
               options->schema[i].name);
      return 0;
    }
    if (!validate_column(current_item, options->schema[i], DATA)) {
      snprintf(error_buffer, ERROR_BUFFER_SIZE,
               "Datatype mismatch: column \"%s\" should be a %s.",
               options->schema[i].name, options->schema[i].datatype);
      return 0;
    }
    columns_consumed++;

    // enforce geodetic point child columns
    if (strcmp(options->schema[i].datatype, "geodetic point") == 0) {
      write_and_access_child_column_name(options->schema[i].name,
                                         "_latlong_accuracy");
      if (current_item) {
        columns_consumed++;
        if (!validate_column(current_item, options->schema[i],
                             LATLONG_ACCURACY)) {
          snprintf(
              error_buffer, ERROR_BUFFER_SIZE,
              "Datatype mismatch: accuracy sub-column of \"%s\" should be a "
              "double.",
              options->schema[i].name);
          return 0;
        }
      }
      write_and_access_child_column_name(options->schema[i].name, "_altitude");
      if (current_item) {
        columns_consumed++;
        if (!validate_column(current_item, options->schema[i], ALTITUDE)) {
          snprintf(
              error_buffer, ERROR_BUFFER_SIZE,
              "Datatype mismatch: altitude sub-column of \"%s\" should be a "
              "double.",
              options->schema[i].name);
          return 0;
        }
      }
      write_and_access_child_column_name(options->schema[i].name,
                                         "_altitude_accuracy");
      if (current_item) {
        columns_consumed++;
        if (!validate_column(current_item, options->schema[i],
                             ALTITUDE_ACCURACY)) {
          snprintf(error_buffer, ERROR_BUFFER_SIZE,
                   "Datatype mismatch: altitude accuracy sub-column of \"%s\" "
                   "should be "
                   "a double.",
                   options->schema[i].name);
          return 0;
        }
      }
    }

    // enforce columns (optional)
    if (options->schema[i].comments) {

      current_column_name = cur;
      cur_write_column_name(options->schema[i].name);
      cur_memcpy("_comments");

      current_item = json_object_getn(entry, current_column_name,
                                      cur - current_column_name);

      if (current_item) {
        columns_consumed++;
        if (!validate_column(current_item, options->schema[i], COMMENTS)) {
          snprintf(error_buffer, ERROR_BUFFER_SIZE,
                   "Datatype mismatch: comments sub-column of \"%s\" should be "
                   "a string.",
                   options->schema[i].name);
          return 0;
        }
        cur_append(',');
      } else {
        // pull back cur
        n = strlen(options->schema[i].name) + strlen("_comments");
        remaining_size += n;
        cur -= n;
      }
    }
  }

  // make sure there are no extra columns
  if (json_object_size(entry) != columns_consumed) {
    memcpy(error_buffer, "The given entry contains erroneous values.",
           strlen("The given entry contains erroneous values.") + 1);
    return 0;
  }

  // the entry is now considered as valid, we can commit to malloc'ing memory
  // it might be optimizeable by using a dynamic memory pool

  // remove trailing comma & add ") VALUES ("
  cur--;
  remaining_size++;
  cur_memcpy(") VALUES (");

  current_column_name = column_names;
  char *temp_cur = NULL;
  // build VALUES
  if (options->primary_tag) {
    write_next_json_value();
  }

  for (int i = 0; i < options->schema_count; i++) {
    write_next_json_value();

    if (strcmp(options->schema[i].datatype, "geodetic point") == 0) {
      write_next_json_value(); // latlong_accuracy
      write_next_json_value(); // altitude
      write_next_json_value(); // altitude_accuracy
    }
  }

  // remove trailing comma
  cur--;
  remaining_size++;

  n = snprintf(
      cur, remaining_size,
      ") ON CONFLICT (%s) DO UPDATE SET %s = EXCLUDED.%s RETURNING %s;",
      options->duplicate_column_name, options->duplicate_column_name,
      options->duplicate_column_name, options->primary_column_name);
  if (errno) {
    goto end;
  }
  if (remaining_size <= n) {
    errno = ENOMEM;
    goto end;
  }
  cur += n;
  remaining_size -= n;
  *cur = '\0';

  // query database
  sql_query(query, res, conn);
  status = 1;

end:
  if (!status && *error_buffer == '\0') {

    switch (errno) {
    case ENOMEM:
      memcpy(error_buffer, "Memory limit exceeded.\n",
             strlen("Memory limit exceeded.\n") + 1);
      break;
    case EINVAL:
      memcpy(error_buffer, "Invalid character encoding.\n",
             strlen("Invalid character encoding.\n") + 1);
      break;
    default:
      memcpy(
          error_buffer,
          "Something went wrong while preparing an INSERT statement.\n",
          strlen(
              "Something went wrong while preparing an INSERT statement.\n") +
              1);
      break;
    }
  }
  if (!status) {
    if (errno) {
      perror("lkjahsdf");
      puts("lkjhasdlfkjhalkjshdflkjhasdf\n");
    }
    puts("laksjdflhasdf\n");
  }
  return status;
}
