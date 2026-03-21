#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
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

int validate_and_insert_into(
    struct insert_options *options, json_t *entry, PGresult **res,
    PGconn *conn) { // @TODO use iterators, placeholders, and a single loop to
                    // achieve good safety, runtime, and force all schema items
                    // to be included.
  int status = 1;   // innocent until proven guilty
  const char *key = NULL;
  const json_t *value = NULL;
  char query[MAX_INSERT_QUERY_SIZE];
  char *cur = query;
  size_t remaining_size = MAX_INSERT_QUERY_SIZE;
  size_t n;

  // compile regex if needed
  regex_t suffix_preg;
  bool suffix_preg_compiled = false;

  if (regcomp(&suffix_preg,
              "(_comments|_latlong_accuracy|_altitude_accuracy|_altitude)$",
              REG_EXTENDED) == 0)
    suffix_preg_compiled = true;
  else {
    errno = EDOM;
    status = 0;
    goto insert_end;
  }

  // write in the beginning of the query
  // we can guarentee this fits into the query buffer because it's the first
  // text to be inserted and is a known constant.
  n = snprintf(cur, remaining_size, "INSERT INTO %s (", options->table_name);
  if (errno) {
    status = 0;
    goto insert_end;
  }
  if (remaining_size < n) {
    errno = ENOMEM;
    status = 0;
    goto insert_end;
  }
  remaining_size -= n;
  cur += n;

  // check for conformance & write in column names
  json_object_foreach(entry, key, value) {
    // ignore keys with null values. This allows child key existence to be
    // detected while also preventing the need for extra NULL checks down the
    // line.
    if (json_is_null(value))
      continue;

    // skip id column (rely on auto-increment)
    if (strcmp(key, "id") == 0)
      continue;
    bool validated = false;

    // column type & suffix related variables. These need not be freed.
    enum column_type column_type = DATA;
    regmatch_t suffix_matches[1 + 1];

    // check if the column is directly inside the schema or not.
    // check if it's a special column (i.e. sub-column)
    if (regexec(&suffix_preg, key, 2, suffix_matches, 0) != REG_NOMATCH) {
      const char *suffix = key + suffix_matches[1].rm_so;

      if (strcmp(suffix, "_comments") == 0) {
        column_type = COMMENTS;
      } else if (strcmp(suffix, "_latlong_accuracy") == 0) {
        column_type = LATLONG_ACCURACY;
      } else if (strcmp(suffix, "_altitude_accuracy") == 0) {
        column_type = ALTITUDE_ACCURACY;
      } else if (strcmp(suffix, "_altitude") == 0) {
        column_type = ALTITUDE;
      } else {
        errno = EDOM;
        fprintf(stderr, "CRITICAL: Failed to trap column suffix.\n");
        status = 0;
        goto insert_end;
      }
    }

    n = strlen(key);

    if (remaining_size < n + 1) {
      errno = ENOMEM;
      status = 0;
      goto insert_end;
    }
    remaining_size -= n + 1;
    memcpy(cur, key, n);
    // temporarily null terminate
    *(cur + n) = '\0';

    // validate entry data
    if (strcmp(cur, "primary_tag") == 0) {
      // it's hard to validate FOREIGN KEY so we'll let Postgres take care of
      // this.
      if (!json_is_integer(value)) {
        status = 0;
        goto insert_end;
      }
    } else {
      for (int i = 0; i < options->schema_count; i++) {
        // find which entry in the schema matches

        if (str_cci_cmp(cur, options->schema[i].name) == 0) {
          // validate against the column schema
          if (!validate_column(value, options->schema[i], column_type)) {
            status = 0;
            goto insert_end;
          }

          goto valid_column;
        }
      }

      status = 0;
      goto insert_end;
    }
  valid_column:
    // convert column_name into lower_snake_case
    to_lower_snake_case(cur);
    cur += n;
    *cur++ = ',';
  }

  // remove trailing comma & add ") VALUES ("
  cur--;
  remaining_size++;
  n = strlen(") VALUES (");
  if (remaining_size < n) {
    errno = ENOMEM;
    status = 0;
    goto insert_end;
  }
  remaining_size -= n;
  memcpy(cur, ") VALUES (", n);
  cur += n;

  // build the query
  json_object_foreach(entry, key, value) {
    if (json_is_null(value))
      continue;

    // skip ID column (rely on auto-increment)
    if (strcmp(key, "id") == 0)
      continue;

    n = write_json_value(value, cur, remaining_size);
    if (remaining_size <= n) {
      errno = ENOMEM;
      status = 0;
      goto insert_end;
    }
    remaining_size -= n;
    cur += n;

    *cur++ = ',';
    remaining_size--;
  }

  // remove trailing commas
  cur--;
  remaining_size++;

  n = snprintf(
      cur, remaining_size,
      ") ON CONFLICT (%s) DO UPDATE SET %s = EXCLUDED.%s RETURNING %s;",
      options->duplicate_column_name, options->duplicate_column_name,
      options->duplicate_column_name, options->primary_column_name);
  if (errno) {
    status = 0;
    goto insert_end;
  }
  if (remaining_size <= n) {
    errno = ENOMEM;
    status = 0;
    goto insert_end;
  }

  cur += n;
  remaining_size -= n;
  *cur = '\0';
  // printf("%ld %ld\n", MAX_INSERT_QUERY_SIZE + query - cur, remaining_size);

  // query database
  sql_query(query, res, conn);

insert_end:
  if (suffix_preg_compiled)
    regfree(&suffix_preg);

  return status;
}
