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
#define write_and_validate_column(column_name)                                 \
  ({                                                                           \
    current_column_name = cur;                                                 \
    cur_write_column_name(column_name);                                        \
    current_item = json_object_getn(entry, current_column_name,                \
                                    cur - current_column_name);                \
    if (!current_item) {                                                       \
      goto end;                                                                \
    }                                                                          \
  })
#define write_and_validate_child_column_name(column_name, column_suffix)       \
  ({                                                                           \
    current_column_name = cur;                                                 \
    cur_write_column_name(column_name);                                        \
    cur_memcpy(column_suffix);                                                 \
    current_item = json_object_getn(entry, current_column_name,                \
                                    cur - current_column_name);                \
    if (!current_item) {                                                       \
      goto end;                                                                \
    }                                                                          \
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

int validate_and_insert_into(struct insert_options *options, json_t *entry,
                             PGresult **res, PGconn *conn) {
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
    current_item = json_object_get(entry, "primary_tag");
    if (!current_item)
      return 0;
    if (!json_is_integer(current_item))
      return 0;

    cur_write_column_name("primary_tag");
    cur_append(',');
    columns_consumed++;
  }

  for (int i = 0; i < options->schema_count; i++) {
    // @TODO add optionality
    write_and_validate_column(options->schema[i].name);
    if (!validate_column(current_item, options->schema[i], DATA)) {
      return 0;
    }
    cur_append(',');
    columns_consumed++;

    // enforce geodetic point child columns
    if (strcmp(options->schema[i].datatype, "geodetic point") == 0) {
      write_and_validate_child_column_name(options->schema[i].name,
                                           "_latlong_accuracy,");
      if (!validate_column(current_item, options->schema[i], LATLONG_ACCURACY))
        return 0;
      write_and_validate_child_column_name(options->schema[i].name,
                                           "_altitude,");
      if (!validate_column(current_item, options->schema[i], ALTITUDE))
        return 0;
      write_and_validate_child_column_name(options->schema[i].name,
                                           "_altitude_accuracy,");
      if (!validate_column(current_item, options->schema[i], ALTITUDE_ACCURACY))
        return 0;

      columns_consumed += 3;
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
        if (!validate_column(current_item, options->schema[i], COMMENTS))
          return 0;
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
    return 0;
  }

  // the entry is now considered as valid, we can commit to malloc'ing memory
  // it might be optimizeable by using a dynamic memory pool

  // remove trailing comma & add ") VALUES ("
  cur--;
  remaining_size++;
  cur_memcpy(") VALUES (");

  // build VALUES
  if (options->primary_tag) {
    cur_write_json_value(json_object_get(entry, "primary_tag"));
    cur_append(',');
  }

  current_column_name = column_names;
  char *temp_cur = NULL;
  for (int i = 0; i < options->schema_count; i++) {
    temp_cur = current_column_name;
    while (*temp_cur != ',' && *temp_cur != ')')
      temp_cur++;
    current_item = json_object_getn(entry, current_column_name,
                                    current_column_name - temp_cur);
    current_column_name = temp_cur + 1;
    cur_write_json_value(current_item);
    cur_append(',');
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
  // printf("%ld %ld\n", MAX_INSERT_QUERY_SIZE + query - cur, remaining_size);

  // query database
  sql_query(query, res, conn);
  status = 1;

end:
  return status;
}
