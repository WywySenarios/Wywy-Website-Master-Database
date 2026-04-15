#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#ifndef CUR_LIB
#define CUR_LIB
#include "utils/cur.h"
#endif
#include "logging.h"
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

#define MAX_INSERT_QUERY_SIZE 8192
#define MAX_PLACEHOLDER_SIZE 65536
#define MAX_COLUMN_NAME_SIZE 100
#define MAX_COLUMNS 64
#define LARGEST_COLUMN_NAME_SUFFIX strlen("_altitude_accuracy")

// does NOT validate datatypes
// @TODO do not automatically advance cur?
#define write_and_access_column(cur, remaining_size, cur_checkpoint,           \
                                column_name)                                   \
  ({                                                                           \
    cur_checkpoint = cur;                                                      \
    cur_write_column_name(cur, remaining_size, column_name);                   \
    current_item =                                                             \
        json_object_getn(entry, cur_checkpoint, cur - cur_checkpoint);         \
    cur_append(cur, remaining_size, ',');                                      \
  })
#define write_and_access_child_column_name(                                    \
    cur, remaining_size, cur_checkpoint, column_name, column_suffix)           \
  ({                                                                           \
    cur_checkpoint = cur;                                                      \
    cur_write_column_name(cur, remaining_size, column_name);                   \
    cur_memcpy(cur, remaining_size, column_suffix);                            \
    current_item =                                                             \
        json_object_getn(entry, cur_checkpoint, cur - cur_checkpoint);         \
    cur_append(cur, remaining_size, ',');                                      \
  })
#define cur_write_json_value(cur, remaining_size, value)                       \
  ({                                                                           \
    n = write_json_value(value, cur, remaining_size);                          \
    if (remaining_size <= n) {                                                 \
      errno = ENOMEM;                                                          \
      goto end;                                                                \
    }                                                                          \
    remaining_size -= n;                                                       \
    cur += n;                                                                  \
  })

#define write_placeholder_value()                                              \
  do {                                                                         \
    if (current_item != NULL && !json_is_null(current_item)) {                 \
      placeholder_ptrs[columns_consumed++] = placeholder_cur;                  \
      cur_write_json_value(placeholder_cur, placeholder_remaining_size,        \
                           current_item);                                      \
      cur_append(placeholder_cur, placeholder_remaining_size, '\0');           \
    } else {                                                                   \
      placeholder_ptrs[columns_consumed++] = NULL;                             \
    }                                                                          \
  } while (0)

int validate_and_insert_into(struct insert_options *options, json_t *entry,
                             PGresult **res, PGconn *conn, char *error_buffer) {
  // @TODO use placeholders
  int status = 0; // innocent until proven guilty
  const char *key = NULL;
  const json_t *value = NULL;
  char query[MAX_INSERT_QUERY_SIZE];
  char placeholders[MAX_PLACEHOLDER_SIZE];
  const char *placeholder_ptrs[MAX_COLUMNS];
  char *current_column_name = NULL;
  char *query_cur = query;
  char *placeholder_cur = placeholders;
  size_t query_remaining_size = MAX_INSERT_QUERY_SIZE;
  size_t placeholder_remaining_size = MAX_PLACEHOLDER_SIZE;
  size_t n;
  int primary_column_present = 0;
  *error_buffer = '\0';

  // write in the beginning of the query
  // we can guarentee this fits into the query buffer because it's the first
  // text to be inserted and is a known constant.
  n = snprintf(query_cur, query_remaining_size, "INSERT INTO %s (",
               options->table_name);
  if (errno) {
    goto end;
  }
  if (query_remaining_size < n) {
    errno = ENOMEM;
    goto end;
  }
  query_remaining_size -= n;
  query_cur += n;

  // check for conformance & write in column names
  size_t columns_consumed = 0;
  json_t *current_item = NULL;
  char *query_cur_checkpoint = query_cur;
  // check for UPSERT id column
  if (!options->primary_column_in_schema) {
    write_and_access_column(query_cur, query_remaining_size,
                            query_cur_checkpoint, options->primary_column_name);
    if (current_item) {
      if (!json_is_integer(current_item)) {
        memcpy(
            error_buffer,
            "Datatype datatype: the primary column should be an integer.",
            strlen(
                "Datatype mismatch: the primary column should be an integer.") +
                1);
        return 0;
      }
      write_placeholder_value();
      primary_column_present = 1;
    } else {
      revert_cur_checkpoint(query_cur, query_remaining_size,
                            query_cur_checkpoint);
    }
  }

  // check for primary_tag
  if (options->primary_tag) {
    write_and_access_column(query_cur, query_remaining_size,
                            query_cur_checkpoint, "primary_tag");

    if (!current_item) {
      memcpy(error_buffer, "Missing column \"primary tag\".",
             strlen("Missing column \"primary tag\".") + 1);
      return 0;
    }
    if (!json_is_integer(current_item)) {
      memcpy(
          error_buffer, "Datatype mismatch: primary_tag should be an integer.",
          strlen("Datatype mismatch: primary_tag should be an integer.") + 1);
      return 0;
    }
    write_placeholder_value();
  }

  for (int i = 0; i < options->schema_count; i++) {
    // @TODO add optionality
    write_and_access_column(query_cur, query_remaining_size,
                            query_cur_checkpoint, options->schema[i].name);
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
    write_placeholder_value();

    // enforce geodetic point child columns
    if (strcmp(options->schema[i].datatype, "geodetic point") == 0) {
      write_and_access_child_column_name(
          query_cur, query_remaining_size, query_cur_checkpoint,
          options->schema[i].name, "_latlong_accuracy");
      if (current_item) {
        if (!validate_column(current_item, options->schema[i],
                             LATLONG_ACCURACY)) {
          snprintf(
              error_buffer, ERROR_BUFFER_SIZE,
              "Datatype mismatch: accuracy sub-column of \"%s\" should be a "
              "double.",
              options->schema[i].name);
          return 0;
        }
        write_placeholder_value();
      } else {
        revert_cur_checkpoint(query_cur, query_remaining_size,
                              query_cur_checkpoint);
      }
      write_and_access_child_column_name(query_cur, query_remaining_size,
                                         query_cur_checkpoint,
                                         options->schema[i].name, "_altitude");
      if (current_item) {
        if (!validate_column(current_item, options->schema[i], ALTITUDE)) {
          snprintf(
              error_buffer, ERROR_BUFFER_SIZE,
              "Datatype mismatch: altitude sub-column of \"%s\" should be a "
              "double.",
              options->schema[i].name);
          return 0;
        }
        write_placeholder_value();
      } else {
        revert_cur_checkpoint(query_cur, query_remaining_size,
                              query_cur_checkpoint);
      }
      write_and_access_child_column_name(
          query_cur, query_remaining_size, query_cur_checkpoint,
          options->schema[i].name, "_altitude_accuracy");
      if (current_item) {
        if (!validate_column(current_item, options->schema[i],
                             ALTITUDE_ACCURACY)) {
          snprintf(error_buffer, ERROR_BUFFER_SIZE,
                   "Datatype mismatch: altitude accuracy sub-column of \"%s\" "
                   "should be "
                   "a double.",
                   options->schema[i].name);
          return 0;
        }
        write_placeholder_value();
      } else {
        revert_cur_checkpoint(query_cur, query_remaining_size,
                              query_cur_checkpoint);
      }
    }

    // enforce columns (optional)
    if (options->schema[i].comments) {
      write_and_access_child_column_name(query_cur, query_remaining_size,
                                         query_cur_checkpoint,
                                         options->schema[i].name, "_comments");

      if (current_item) {
        if (!validate_column(current_item, options->schema[i], COMMENTS)) {
          snprintf(error_buffer, ERROR_BUFFER_SIZE,
                   "Datatype mismatch: comments sub-column of \"%s\" should be "
                   "a string.",
                   options->schema[i].name);
          return 0;
        }
        write_placeholder_value();
      } else {
        revert_cur_checkpoint(query_cur, query_remaining_size,
                              query_cur_checkpoint);
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
  cur_shave(query_cur, query_remaining_size, 1);
  cur_memcpy(query_cur, query_remaining_size, ") VALUES (");

  // write in all the placeholders
  for (int i = 1; i < 10 && i <= columns_consumed; i++) {
    cur_append(query_cur, query_remaining_size, '$');
    cur_append(query_cur, query_remaining_size, '0' + i);
    cur_append(query_cur, query_remaining_size, ',');
  }
  // assume that the maximum number of columns consumed is 64 (i can take a
  // maximum value of 65) i.e. assume the table schema has a valid number of
  // columns.
  for (int i = 10; i <= columns_consumed; i++) {
    cur_append(query_cur, query_remaining_size, '$');
    cur_append(query_cur, query_remaining_size, '0' + i / 10);
    cur_append(query_cur, query_remaining_size, '0' + i % 10);
    cur_append(query_cur, query_remaining_size, ',');
  }

  // remove trailing comma
  cur_shave(query_cur, query_remaining_size, 1);

  cur_memcpy(query_cur, query_remaining_size, ") ON CONFLICT (");
  cur_write_column_name(query_cur, query_remaining_size,
                        options->duplicate_column_name);
  cur_memcpy(query_cur, query_remaining_size, ") DO UPDATE SET ");

  // upsert (update all columns on conflict)
  // ignore primary column.
  // for the tag aliases table, the primary column is also a part of the schema.
  // otherwise, the ID column is the conflicting column (i.e. it does not need
  // to be updated). this is fragile design and should be fixed when the need
  // arises. I think this will go unfixed for a very long time.
  if (primary_column_present) {
    cur_write_column_name(query_cur, query_remaining_size,
                          options->primary_column_name);
    cur_memcpy(query_cur, query_remaining_size, " = EXCLUDED.");
    cur_write_column_name(query_cur, query_remaining_size,
                          options->primary_column_name);
    cur_append(query_cur, query_remaining_size, ',');
  }

  // primary tag
  if (options->primary_tag) {
    cur_memcpy(query_cur, query_remaining_size,
               "primary_tag = EXCLUDED.primary_tag,");
  }

  // handle the bulk data
  for (int i = 0; i < options->schema_count; i++) {
    cur_write_column_name(query_cur, query_remaining_size,
                          options->schema[i].name);
    cur_memcpy(query_cur, query_remaining_size, " = EXCLUDED.");
    cur_write_column_name(query_cur, query_remaining_size,
                          options->schema[i].name);
    cur_append(query_cur, query_remaining_size, ',');

    if (strcmp(options->schema[i].datatype, "geodetic point") == 0) {
      cur_write_column_name(query_cur, query_remaining_size,
                            options->schema[i].name);
      cur_memcpy(query_cur, query_remaining_size,
                 "_latlong_accuracy = EXCLUDED.");
      cur_write_column_name(query_cur, query_remaining_size,
                            options->schema[i].name);
      cur_memcpy(query_cur, query_remaining_size, "_latlong_accuracy,");
      cur_write_column_name(query_cur, query_remaining_size,
                            options->schema[i].name);
      cur_memcpy(query_cur, query_remaining_size, "_altitude = EXCLUDED.");
      cur_write_column_name(query_cur, query_remaining_size,
                            options->schema[i].name);
      cur_memcpy(query_cur, query_remaining_size, "_altitude,");
      cur_write_column_name(query_cur, query_remaining_size,
                            options->schema[i].name);
      cur_memcpy(query_cur, query_remaining_size,
                 "_altitude_accuracy = EXCLUDED.");
      cur_write_column_name(query_cur, query_remaining_size,
                            options->schema[i].name);
      cur_memcpy(query_cur, query_remaining_size, "_altitude_accuracy,");
    }

    if (options->schema[i].comments) {
      cur_write_column_name(query_cur, query_remaining_size,
                            options->schema[i].name);
      cur_memcpy(query_cur, query_remaining_size, "_comments = EXCLUDED.");
      cur_write_column_name(query_cur, query_remaining_size,
                            options->schema[i].name);
      cur_memcpy(query_cur, query_remaining_size, "_comments,");
    }
  }

  // remove trailing comma
  cur_shave(query_cur, query_remaining_size, 1);

  cur_memcpy(query_cur, query_remaining_size, " RETURNING ");
  cur_write_column_name(query_cur, query_remaining_size,
                        options->primary_column_name);
  cur_append(query_cur, query_remaining_size, ';');
  cur_append(query_cur, query_remaining_size, '\0');

  // query database
  if (getenv("SQL_RECEPTIONIST_LOG_QUERIES") &&
      strcmp(getenv("SQL_RECEPTIONIST_LOG_QUERIES"), "TRUE") == 0)
    log_debug_printf("INSERT query: %s\n", query);

  // Submit & Execute query
  *res = PQexecParams(conn, query, columns_consumed, NULL, placeholder_ptrs,
                      NULL, NULL, 0);
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
  return status;
}
