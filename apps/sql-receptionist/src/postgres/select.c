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
  n = strlen("SELECT ");
  if (remaining_size < n) {
    errno = ENOMEM;
    return;
  }
  remaining_size -= n;
  memcpy(cur, "SELECT ", n);
  cur += n;

  // columns
  // id column
  if (options->id_column) {
    if (remaining_size < 3) {
      errno = ENOMEM;
      return;
    }
    remaining_size -= 3;
    *cur++ = 'i';
    *cur++ = 'd';
    *cur++ = ',';
  }

  // primary tag column
  if (options->primary_tag) {
    n = strlen("primary_tag,");
    if (remaining_size < n) {
      errno = ENOMEM;
      return;
    }
    remaining_size -= n;
    memcpy(cur, "primary_tag,", n);
    cur += n;
  }

  for (int i = 0; i < options->schema_count; i++) {
    size_t column_name_size = strlen(options->schema[i].name);
    size_t temp;
    // add "[column_name],"
    if (strcmp(options->schema[i].datatype, "geodetic point") ==
        0) { // special geodetic point logic
      // ST_AsText([main_column]), [latlong_accuracy], [altitude],
      // [altitude_accuracy],
      n = 5 * column_name_size + strlen("ST_AsText") + strlen(" AS ") +
          strlen("_latlong_accuracy") + strlen("_altitude") +
          strlen("_altitude_accuracy") + 4;
      if (remaining_size < n) {
        errno = ENOMEM;
        return;
      }
      remaining_size -= n;
      // main column
      memcpy(cur, "ST_AsText(", temp = strlen("ST_AsText("));
      cur += temp;
      memcpy(cur, options->schema[i].name, column_name_size);
      to_lower_snake_case_n(cur, column_name_size);
      cur += column_name_size;
      *cur++ = ')';
      *cur++ = ' ';
      *cur++ = 'A';
      *cur++ = 'S';
      *cur++ = ' ';
      memcpy(cur, options->schema[i].name, column_name_size);
      to_lower_snake_case_n(cur, column_name_size);
      cur += column_name_size;
      *cur++ = ',';
      // latlong_accuracy
      memcpy(cur, options->schema[i].name, column_name_size);
      to_lower_snake_case_n(cur, column_name_size);
      cur += column_name_size;
      memcpy(cur, "_latlong_accuracy", temp = strlen("_latlong_accuracy"));
      cur += temp;
      *cur++ = ',';
      // altitude
      memcpy(cur, options->schema[i].name, column_name_size);
      to_lower_snake_case_n(cur, column_name_size);
      cur += column_name_size;
      memcpy(cur, "_altitude", temp = strlen("_altitude"));
      cur += temp;
      *cur++ = ',';
      // altitude_accuracy
      memcpy(cur, options->schema[i].name, column_name_size);
      to_lower_snake_case_n(cur, column_name_size);
      cur += column_name_size;
      memcpy(cur, "_altitude_accuracy", temp = strlen("_altitude_accuracy"));
      cur += temp;
      *cur++ = ',';
    } else {
      if (remaining_size < (n = column_name_size + 1)) {
        errno = ENOMEM;
        return;
      }
      remaining_size -= n;
      memcpy(cur, options->schema[i].name, column_name_size);
      to_lower_snake_case_n(cur, column_name_size);
      cur += column_name_size;
      *cur++ = ',';
    }

    // comments column
    if (options->schema[i].comments) {
      n = column_name_size + strlen("_comments") + 1;
      if (remaining_size < n) {
        errno = ENOMEM;
        return;
      }
      remaining_size -= n;
      memcpy(cur, options->schema[i].name, column_name_size);
      to_lower_snake_case_n(cur, column_name_size);
      cur += column_name_size;
      memcpy(cur, "_comments", temp = strlen("_comments"));
      cur += temp;
      *cur++ = ',';
    }
  }

  // get rid of trailing comma
  cur--;
  remaining_size++;

  n = snprintf(cur, remaining_size, " FROM %s ORDER BY %s %s LIMIT %d;",
               options->table_name, options->order_by_column,
               options->order_by_order, options->limit);
  if (n < 0 || errno == EILSEQ) {
    errno = EILSEQ;
    return;
  }
  cur += n;
  if (remaining_size < n) {
    errno = ENOMEM;
    return;
  }
  remaining_size -= n;

  // row offset
  if (options->row_offset) {
    // -- to get rid of the semi-colon currently written into the query.

    n = snprintf(cur, remaining_size, " OFFSET %d;", options->row_offset);
    if (n < 0 || errno == EILSEQ) {
      errno = EILSEQ;
      return;
    }
    if (remaining_size < n) {
      errno = ENOMEM;
      return;
    }
    remaining_size -= n;
    cur += n;
  }

  // null termination
  if (remaining_size < 1) {
    errno = ENOMEM;
    return;
  }
  remaining_size--;
  *cur++ = '\0';
}

int serialize_select_result(const PGresult *res, char *buffer,
                            const size_t buffer_size) {
  char *cur = buffer;
  size_t remaining_size = buffer_size;
  size_t n = 0;

  // column names
  n = strlen("{\"columns\":[");
  if (remaining_size < n) {
    errno = ENOMEM;
    return -1;
  }
  remaining_size -= n;
  memcpy(cur, "{\"columns\":[", n);
  cur += n;

  for (int i = 0; i < PQnfields(res); i++) {
    if (remaining_size < 1) {
      errno = ENOMEM;
      return -1;
    }
    remaining_size--;
    *cur++ = '"';
    n = strlen(PQfname(res, i));
    if (n + 2 > remaining_size) { // account for both n and the extra text after
      errno = ENOMEM;
      return -1;
    }
    remaining_size -= n + 2;
    memcpy(cur, PQfname(res, i), n);
    cur += n;
    *cur++ = '"';
    *cur++ = ',';
  }

  // remove trailing comma & continue with the data section
  n = strlen("],\"data\":[");
  if (remaining_size < n - 1) {
    errno = ENOMEM;
    return -1;
  }
  memcpy(--cur, "],\"data\":[", n);
  remaining_size -= n - 1;
  cur += n;
  // catch edge case where there's no data
  if (PQntuples(res) == 0) {
    if (remaining_size < 1) {
      errno = ENOMEM;
      return -1;
    }
    remaining_size--;
    cur++;
  } else {
    for (int row_num = 0; row_num < PQntuples(res); row_num++) {
      if (remaining_size < 1) {
        errno = ENOMEM;
        return -1;
      }
      remaining_size--;
      *cur++ = '[';
      for (int col_num = 0; col_num < PQnfields(res); col_num++) {
        if (PQgetisnull(res, row_num, col_num)) {
          n = strlen("null,");
          if (remaining_size < n) {
            errno = ENOMEM;
            return -1;
          }
          memcpy(cur, "null,", n);
          remaining_size -= n;
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
          if (remaining_size < n + 2) {
            errno = ENOMEM;
            return -1;
          }
          remaining_size -= n + 2;
          *cur++ = '"';
          memcpy(cur, PQgetvalue(res, row_num, col_num), n);
          cur += n;
          *cur++ = '"';
          break;
        default:
          if (remaining_size < n) {
            errno = ENOMEM;
            return -1;
          }
          remaining_size -= n;
          memcpy(cur, PQgetvalue(res, row_num, col_num), n);
          cur += n;
          break;
        }

        if (remaining_size < 1) {
          errno = ENOMEM;
          return -1;
        }
        remaining_size--;
        *cur++ = ',';
      }

      if (remaining_size < 1) {
        errno = ENOMEM;
        return -1;
      }
      remaining_size--;
      cur[-1] = ']';
      *cur++ = ',';
    }
  }

  // close out the JSON by overwriting the last trailing comma
  if (remaining_size < 2) {
    errno = ENOMEM;
    return -1;
  }
  remaining_size -= 2;
  cur[-1] = ']';
  *cur++ = '}';
  *cur++ = '\0';

  // do not count the null terminator (i.e. -1)
  return buffer_size - remaining_size - 1;
}