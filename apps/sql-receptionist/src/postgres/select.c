#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#include "libpq-fe.h"
#include <asm-generic/errno.h>
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

  return query_size;
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
  if (n < 0 || errno == EILSEQ) {
    errno = EILSEQ;
    return;
  }
  cur += n;
  remaining_size -= n;
  if (remaining_size < 0) {
    errno = ENOMEM;
    return;
  }

  // row offset
  if (options->row_offset) {
    // -- to get rid of the semi-colon currently written into the query.

    n = snprintf(--cur, remaining_size, " OFFSET %d;", options->row_offset);
    if (n < 0 || errno == EILSEQ) {
      errno = EILSEQ;
      return;
    }
    remaining_size -= n;
    if (remaining_size < 0) {
      errno = ENOMEM;
      return;
    }
  }
}

void serialize_select_result(const PGresult *res, char *buffer,
                             const size_t buffer_size) {
  char *cur = buffer;
  size_t remaining_size = buffer_size;
  size_t n = 0;

  // column names
  n = strlen("{\"columns\":[");
  if ((remaining_size -= n) < 0) {
    errno = ENOMEM;
    return;
  }
  memcpy(cur, "{\"columns\":[", n);
  cur += n;

  for (int i = 0; i < PQnfields(res); i++) {
    if (--remaining_size < 0) {
      errno = ENOMEM;
      return;
    }
    *cur++ = '"';
    n = strlen(PQfname(res, i));
    if (n >
        (remaining_size - 2)) { // account for both n and the extra text after
      errno = ENOMEM;
      return;
    }
    memcpy(cur, PQfname(res, i), n);
    cur += n;
    remaining_size -= n;

    *cur++ = '"';
    *cur++ = ',';
    remaining_size -= 2;

    if (remaining_size < 1) {
      errno = ENOMEM;
      return;
    }
    // remove trailing comma & continue with the data section
    n = strlen("],\"data\":[") - 1;
    if ((remaining_size -= n) < 0) {
      errno = ENOMEM;
      return;
    }
    memcpy(--cur, "],\"data\":[", n);
    cur += n;
    for (int row_num = 0; row_num < PQntuples(res); row_num++) {
      if (--remaining_size < 0) {
        errno = ENOMEM;
        return;
      }
      *cur++ = '[';
      for (int col_num = 0; col_num < PQnfields(res); col_num++) {
        if (PQgetisnull(res, row_num, col_num)) {
          n = strlen("null,");
          if ((remaining_size -= n) < 0) {
            errno = ENOMEM;
            return;
          }
          memcpy(cur, "null,", n);
          cur += n;
          continue;
        }

        n = strlen(PQgetvalue(res, row_num, col_num));
        switch (PQftype(res, col_num)) {
        case 25:
        case 1082:
        case 1083:
        case 1114:
          // if the type requires quotation,
          if ((remaining_size -= n + 2) < 0) {
            errno = ENOMEM;
            return;
          }
          *cur++ = '"';
          memcpy(cur, PQgetvalue(res, row_num, col_num), n);
          cur += n;
          *cur++ = '"';
          break;
        default:
          if ((remaining_size -= n) < 0) {
            errno = ENOMEM;
            return;
          }
          memcpy(cur, PQgetvalue(res, row_num, col_num), n);
          cur += n;
          break;
        }

        *cur++ = ',';
        remaining_size--;
      }
    }

    if ((remaining_size -= 2) < 0) {
      errno = ENOMEM;
      return;
    }
    cur[0] = ']';
    cur[1] = ',';
    cur += 2;
  }

  // close out the JSON by overwriting the last trailing comma
  cur[-1] = '}';
}