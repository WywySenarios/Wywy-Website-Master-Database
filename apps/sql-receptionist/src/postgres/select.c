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

size_t select_query_size(struct select_options *options) {
  // do not validate options.
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
}

void construct_select_query(struct select_options *options, char *buffer,
                            size_t buffer_size) {
  // do not validate options.
  char *cur = buffer;
  size_t n = 0;
  size_t remaining_size = buffer_size;

  // construct query
  n = strlen("SELECT (");
  if (remaining_size < n) {
    errno = ENOMEM;
    return;
  }
  memcpy(cur, "SELECT (", n);
  cur += n;
  remaining_size -= n;

  // columns
  for (int i = 0; i < options->schema_count; i++) {
    // add "[column_name],"
    n = strlen(options->schema[i].name);
    if (remaining_size < n + 1) {
      errno = ENOMEM;
      return;
    }
    memcpy(cur, options->schema[i].name, n);
    *++cur = ',';
    remaining_size -= n + 1;
  }
  *cur = ')'; // get rid of trailing comma & close the bracket

  n = snprintf(cur, remaining_size, " FROM %s\nORDER BY %s %s\nLIMIT %d;",
               options->table_name, options->order_by_column,
               options->order_by_order, options->limit);
  cur += n;
  remaining_size -= n;
  if (remaining_size < 0) {
    errno = ENOMEM;
    return;
  }

  // row offset
  if (options->row_offset) {
    // -- to get rid of the semi-colon currently written into the query.
    remaining_size -=
        snprintf(--cur, remaining_size, " OFFSET %d;", options->row_offset);
    if (remaining_size < 0) {
      errno = ENOMEM;
      return;
    }
  }
}