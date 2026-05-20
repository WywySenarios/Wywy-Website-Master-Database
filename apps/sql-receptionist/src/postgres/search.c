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
#include "postgres/search.h"
#include "server/responses.h"
#include "utils/format_string.h"
#include <errno.h>
#include <libpq-fe.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEARCH_RESPONSE_SIZE 262144

enum search_table_type detect_search_table_type(struct db *database,
                                                const char *raw_table_name) {
  size_t len = strlen(raw_table_name);

  struct suffix_entry {
    const char *suffix;
    size_t suffix_len;
    enum search_table_type type;
  };

  static const struct suffix_entry suffixes[] = {
      {"_tag_aliases", 12, SEARCH_TAG_ALIASES},
      {"_descriptors", 12, SEARCH_DESCRIPTORS},
      {"_tag_groups", 11, SEARCH_TAG_GROUPS},
      {"_tag_names", 10, SEARCH_TAG_NAMES},
      {"_tags", 5, SEARCH_TAGS},
  };
  static const int num_suffixes = sizeof(suffixes) / sizeof(suffixes[0]);

  for (int i = 0; i < num_suffixes; i++) {
    if (len > suffixes[i].suffix_len &&
        strcmp(raw_table_name + len - suffixes[i].suffix_len,
               suffixes[i].suffix) == 0) {

      if (suffixes[i].type == SEARCH_DESCRIPTORS) {
        int found = 0;
        for (unsigned int t = 0; t < database->tables_count && !found; t++) {
          for (unsigned int d = 0;
               d < database->tables[t].descriptors_count && !found; d++) {
            char expected[256];
            snprintf(expected, sizeof(expected), "%s_%s_descriptors",
                     database->tables[t].table_name,
                     database->tables[t].descriptors[d].name);
            if (strcmp(raw_table_name, expected) == 0) {
              found = 1;
            }
          }
        }
        if (!found)
          continue;
      } else {
        size_t parent_len = len - suffixes[i].suffix_len;
        int found = 0;
        for (unsigned int t = 0; t < database->tables_count && !found; t++) {
          size_t table_len = strlen(database->tables[t].table_name);
          if (table_len == parent_len &&
              strncmp(raw_table_name, database->tables[t].table_name,
                      parent_len) == 0 &&
              database->tables[t].tagging) {
            found = 1;
          }
        }
        if (!found)
          continue;
      }

      return suffixes[i].type;
    }
  }

  for (unsigned int i = 0; i < database->tables_count; i++) {
    if (strcmp(database->tables[i].table_name, raw_table_name) == 0) {
      return SEARCH_MAIN_TABLE;
    }
  }

  return SEARCH_UNKNOWN;
}

int build_search_query(char *query, size_t query_size,
                       enum search_table_type type, const char *table_name,
                       const char *parent_table, const char *display_column,
                       const char *q) {
  if (!q)
    return -1;

  char *endptr = NULL;
  strtol(q, &endptr, 10);
  int q_is_int = (endptr != q && *endptr == '\0');

  const char *id_condition = "";
  if (q_is_int) {
    id_condition = "CAST(id AS TEXT) LIKE $1 OR ";
  }

  char display_col_copy[256];
  size_t dclen = strlen(display_column);
  if (dclen >= sizeof(display_col_copy))
    return -1;
  memcpy(display_col_copy, display_column, dclen + 1);
  to_lower_snake_case(display_col_copy);

  int n = 0;
  switch (type) {
  case SEARCH_MAIN_TABLE:
  case SEARCH_TAG_NAMES:
  case SEARCH_DESCRIPTORS:
    n = snprintf(query, query_size,
                 "SELECT id, %s AS label FROM %s WHERE %sCAST(%s AS TEXT) ILIKE $1 "
                 "ORDER BY id ASC LIMIT %s;",
                 display_col_copy, table_name, id_condition, display_col_copy,
                 getenv("SQL_RECEPTIONIST_SEARCH_LIMIT"));
    break;
  case SEARCH_TAGS:
    n = snprintf(query, query_size,
                 "SELECT tags.id, tag_names.tag_name AS label "
                 "FROM %s_tags AS tags "
                 "JOIN %s_tag_names AS tag_names "
                 "ON tags.tag_id = tag_names.id "
                 "WHERE %sCAST(tag_names.tag_name AS TEXT) ILIKE $1 "
                 "ORDER BY tags.id ASC LIMIT %s;",
                 parent_table, parent_table, id_condition,
                 getenv("SQL_RECEPTIONIST_SEARCH_LIMIT"));
    break;
  case SEARCH_TAG_ALIASES:
    n = snprintf(query, query_size,
                 "SELECT alias AS id, alias AS label "
                 "FROM %s_tag_aliases "
                 "WHERE %sCAST(alias AS TEXT) ILIKE $1 "
                 "ORDER BY alias ASC LIMIT %s;",
                 parent_table, id_condition,
                 getenv("SQL_RECEPTIONIST_SEARCH_LIMIT"));
    break;
  default:
    return -1;
  }

  if (n < 0 || (size_t)n >= query_size)
    return -1;
  return 0;
}

int serialize_search_response(const PGresult *res, char *buffer,
                              size_t buffer_size) {
  char *cur = buffer;
  size_t remaining_size = buffer_size;
  size_t n = 0;

  cur_append(cur, remaining_size, '[');

  int first = 1;
  for (int row = 0; row < PQntuples(res); row++) {
    if (PQgetisnull(res, row, 0) || PQgetisnull(res, row, 1)) {
      continue;
    }

    if (!first) {
      cur_append(cur, remaining_size, ',');
    }
    first = 0;

    cur_memcpy(cur, remaining_size, "{\"id\":");

    const char *id_val = PQgetvalue(res, row, 0);
    int id_is_int = 1;
    for (const char *p = id_val; *p; p++) {
      if (*p < '0' || *p > '9') {
        id_is_int = 0;
        break;
      }
    }

    if (id_is_int) {
      cur_memcpy(cur, remaining_size, id_val);
    } else {
      cur_append(cur, remaining_size, '"');
      cur_memcpy(cur, remaining_size, id_val);
      cur_append(cur, remaining_size, '"');
    }

    cur_memcpy(cur, remaining_size, ",\"label\":\"");
    cur_memcpy(cur, remaining_size, PQgetvalue(res, row, 1));
    cur_memcpy(cur, remaining_size, "\"}");
  }

  cur_append(cur, remaining_size, ']');
  cur_append(cur, remaining_size, '\0');

end:
  if (errno)
    return -1;
  return cur - buffer - 1;
}

void handle_search(int client_fd, PGconn **conn, struct db *database,
                   const char *raw_table_name, const char *q, char **response,
                   size_t *response_len, const char *cookie_header) {
  PGresult *res = NULL;

  if (!q) {
    build_response(400, response, response_len, cookie_header,
                   "Query parameter 'q' is required.");
    goto cleanup;
  }

  if (!*conn) {
    *conn = connect_db(database->db_name);
    if (!*conn) {
      build_response(500, response, response_len, cookie_header,
                     "Failed to connect to database.");
      goto cleanup;
    }
  }

  enum search_table_type type =
      detect_search_table_type(database, raw_table_name);

  if (type == SEARCH_TAG_GROUPS) {
    build_response(400, response, response_len, cookie_header,
                   "Tag groups are not supported for pointer lookup.");
    goto cleanup;
  }

  if (type == SEARCH_UNKNOWN) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Table '%s' not found in database '%s'.",
             raw_table_name, database->db_name);
    build_response(400, response, response_len, cookie_header, msg);
    goto cleanup;
  }

  const char *display_column = NULL;
  char parent_table[64] = "";
  size_t raw_len = strlen(raw_table_name);

  switch (type) {
  case SEARCH_MAIN_TABLE: {
    int found = 0;
    for (unsigned int i = 0; i < database->tables_count && !found; i++) {
      if (strcmp(database->tables[i].table_name, raw_table_name) == 0) {
        found = 1;
        if (database->tables[i].schema_count == 0) {
          char msg[256];
          snprintf(msg, sizeof(msg),
                   "Table '%s' has no columns available for lookup.",
                   raw_table_name);
          build_response(400, response, response_len, cookie_header, msg);
          goto cleanup;
        }
        display_column = database->tables[i].schema[0].name;
      }
    }
    if (!found) {
      build_response(400, response, response_len, cookie_header,
                     "Table not found.");
      goto cleanup;
    }
    break;
  }
  case SEARCH_TAG_NAMES: {
    size_t parent_len = raw_len - 10;
    memcpy(parent_table, raw_table_name, parent_len);
    parent_table[parent_len] = '\0';
    int found = 0;
    for (unsigned int i = 0; i < database->tables_count && !found; i++) {
      if (strcmp(database->tables[i].table_name, parent_table) == 0 &&
          database->tables[i].tagging) {
        found = 1;
      }
    }
    if (!found) {
      build_response(400, response, response_len, cookie_header,
                     "Parent table not found or tagging not enabled.");
      goto cleanup;
    }
    display_column = "tag_name";
    break;
  }
  case SEARCH_TAG_ALIASES: {
    size_t parent_len = raw_len - 12;
    memcpy(parent_table, raw_table_name, parent_len);
    parent_table[parent_len] = '\0';
    int found = 0;
    for (unsigned int i = 0; i < database->tables_count && !found; i++) {
      if (strcmp(database->tables[i].table_name, parent_table) == 0 &&
          database->tables[i].tagging) {
        found = 1;
      }
    }
    if (!found) {
      build_response(400, response, response_len, cookie_header,
                     "Parent table not found or tagging not enabled.");
      goto cleanup;
    }
    display_column = "alias";
    break;
  }
  case SEARCH_TAGS: {
    size_t parent_len = raw_len - 5;
    memcpy(parent_table, raw_table_name, parent_len);
    parent_table[parent_len] = '\0';
    int found = 0;
    for (unsigned int i = 0; i < database->tables_count && !found; i++) {
      if (strcmp(database->tables[i].table_name, parent_table) == 0 &&
          database->tables[i].tagging) {
        found = 1;
      }
    }
    if (!found) {
      build_response(400, response, response_len, cookie_header,
                     "Parent table not found or tagging not enabled.");
      goto cleanup;
    }
    display_column = "tag_name";
    break;
  }
  case SEARCH_DESCRIPTORS: {
    int found = 0;
    for (unsigned int t = 0; t < database->tables_count && !found; t++) {
      for (unsigned int d = 0;
           d < database->tables[t].descriptors_count && !found; d++) {
        char expected[256];
        snprintf(expected, sizeof(expected), "%s_%s_descriptors",
                 database->tables[t].table_name,
                 database->tables[t].descriptors[d].name);
        if (strcmp(raw_table_name, expected) == 0) {
          if (database->tables[t].descriptors[d].schema_count == 0) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Table '%s' has no columns available for lookup.",
                     raw_table_name);
            build_response(400, response, response_len, cookie_header, msg);
            goto cleanup;
          }
          display_column = database->tables[t].descriptors[d].schema[0].name;
          found = 1;
        }
      }
    }
    if (!found) {
      build_response(400, response, response_len, cookie_header,
                     "Descriptor not found.");
      goto cleanup;
    }
    break;
  }
  default:
    build_response(500, response, response_len, cookie_header,
                   "Unexpected table type.");
    goto cleanup;
  }

  char query[65536];
  if (build_search_query(query, sizeof(query), type, raw_table_name,
                         parent_table, display_column, q) != 0) {
    build_response(500, response, response_len, cookie_header,
                   "Failed to build search query.");
    goto cleanup;
  }

  char like_pattern[512];
  snprintf(like_pattern, sizeof(like_pattern), "%%%s%%", q);

  const char *param_values[1] = {like_pattern};
  int param_lengths[1] = {strlen(like_pattern)};
  int param_formats[1] = {0};

  res = PQexecParams(*conn, query, 1, NULL, param_values, param_lengths,
                     param_formats, 0);

  ExecStatusType pqstatus = PQresultStatus(res);
  if (pqstatus != PGRES_TUPLES_OK) {
    build_response_printf(
        500, response, response_len, cookie_header,
        strlen(PQresStatus(pqstatus)) + 2 + strlen(PQerrorMessage(*conn)) + 1,
        "%s: %s", PQresStatus(pqstatus), PQerrorMessage(*conn));
    goto cleanup;
  }

  *response = malloc(SEARCH_RESPONSE_SIZE);
  if (!*response) {
    build_response(500, response, response_len, cookie_header,
                   "Memory allocation failed.");
    goto cleanup;
  }
  *response_len = write_header(200, *response, SEARCH_RESPONSE_SIZE);

  int serialized = serialize_search_response(
      res, *response + *response_len, SEARCH_RESPONSE_SIZE - *response_len);
  if (serialized < 0) {
    free(*response);
    *response = NULL;
    build_response(500, response, response_len, cookie_header,
                   "Search response serialization failed.");
    goto cleanup;
  }
  *response_len += serialized;

cleanup:
  PQclear(res);
}
