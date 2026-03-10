#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#include "postgres/select.h"
#include "utils/format_string.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

extern char *construct_select_query(struct select_options *options) {
  // do not validate options.
  const char *offset_segment = "";

  // calculate expected size
  size_t query_size = strlen("SELECT  FROM \nORDER BY  \nLIMIT ;") +
                      1; // base shape + null terminator

  // query params
  query_size += strlen(options->table_name) + strlen(options->order_by_column) +
                strlen(options->order_by_order);
  // offset
  if (options->row_offset) {
    query_size += int_str_len(options->row_offset);
    query_size += strlen(" OFFSET ;");
  }

  // column names portion i.e. ([...column_names])
  for (int i = 0; i < options->schema_count; i++) {
    query_size += strlen(options->schema[i].name);
  }

  // brackets & commas
  query_size += 2;
  // commas
  query_size +=
      options->schema_count -
      1; // since there are guarenteed to be more characters later, we don't
         // need to worry about buffer overflow for that extra comma.

  char *query = malloc(query_size);
  char *cur = query;
  size_t n = 0;

  // construct query
  n = strlen("SELECT (");
  memcpy(query, "SELECT (", n);
  cur += n;

  // columns
  for (int i = 0; i < options->schema_count; i++) {
    // add "[column_name],"
    memcpy(cur, options->schema[i].name, strlen(options->schema[i].name));
    *++cur = ',';
  }
  *cur = ')'; // get rid of trailing comma & close the bracket

  // write in the rest of the query
  if ((query + query_size) - cur < 0) {
    errno = EINVAL;
    free(query);
    return NULL;
  }
  size_t remaining_size = (query + query_size) - cur;

  cur += snprintf(cur, remaining_size, " FROM %s\nORDER BY %s %s\nLIMIT %d;",
                  options->table_name, options->order_by_column,
                  options->order_by_order, options->limit);

  // row offset
  if (options->row_offset) {
    if (cur > query) {
      errno = EINVAL;
      free(query);
      return NULL;
    }

    if ((query + query_size) - cur < 0) {
      errno = EINVAL;
      free(query);
      return NULL;
    }
    remaining_size = (query + query_size) - cur;

    // -- to get rid of the semi-colon currently written into the query.
    snprintf(--cur, remaining_size, " OFFSET %d;", options->row_offset);
  }

  return query;
}