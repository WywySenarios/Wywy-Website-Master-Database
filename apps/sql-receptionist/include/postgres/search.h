#ifndef SEARCH_HEADER
#define SEARCH_HEADER
#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#include <libpq-fe.h>
#include <stddef.h>

/**
 * Classifies a table name by suffix for search purposes.
 * Suffix mapping: _tags → TAGS, _tag_names → TAG_NAMES,
 * _tag_aliases → TAG_ALIASES, _tag_groups → TAG_GROUPS,
 * _descriptors → DESCRIPTORS, plain name → MAIN_TABLE.
 */
enum search_table_type {
  SEARCH_UNKNOWN,
  SEARCH_MAIN_TABLE,
  SEARCH_TAGS,
  SEARCH_TAG_NAMES,
  SEARCH_TAG_ALIASES,
  SEARCH_TAG_GROUPS,
  SEARCH_DESCRIPTORS,
};

/**
 * Resolves a lower_snake_cased table name to a search_table_type.
 * Suffix-based detection with parent-table verification for tag subtypes.
 * Descriptors are disambiguated by brute-force iterating the database config.
 * @param database The database config containing the tables list.
 * @param raw_table_name The table name, already lower_snake_cased.
 * @returns SEARCH_UNKNOWN if the table cannot be resolved.
 */
extern enum search_table_type
detect_search_table_type(struct db *database, const char *raw_table_name);

/**
 * Builds a parameterized SQL query for the given search table type.
 * Pattern A (main/descriptors/tag_names): SELECT id, {display_col} FROM {table}
 *   WHERE {id_condition}{display_col} ILIKE $1 ORDER BY id ASC LIMIT {limit}.
 * Pattern B (tags): JOIN to {parent}_tag_names, filter on tag_name.
 * Pattern C (tag_aliases): alias = id = label, no id column.
 * id_condition adds CAST(id AS TEXT) LIKE $1 when q is integer-parseable.
 * @param query Output buffer for the generated SQL.
 * @param query_size Size of the output buffer.
 * @param type The search table type.
 * @param table_name Raw table name (for main tables / descriptors).
 * @param parent_table Parent table name (for tag subtypes).
 * @param display_column The column to display as the label.
 * @param q The raw search term (used to decide id_condition).
 * @returns 0 on success, or -1 on error (null q, unknown type, truncation).
 */
extern int build_search_query(char *query, size_t query_size,
                              enum search_table_type type,
                              const char *table_name, const char *parent_table,
                              const char *display_column, const char *q);

/**
 * Serializes a PGresult into JSON format: [{"id":...,"label":"..."},...].
 * id is unquoted for integers (PQgetisnull-checked) and quoted for text.
 * id comes from column 0, label from column 1.
 * Null rows are skipped. Empty results produce [].
 * Trailing comma is cleaned up.
 * @param res The PGresult to serialize.
 * @param buffer Output buffer for the JSON string.
 * @param buffer_size Size of the output buffer.
 * @returns The string length of the serialized JSON or -1 on truncation.
 */
extern int serialize_search_response(const PGresult *res, char *buffer,
                                     size_t buffer_size);

/**
 * Top-level handler for GET /{db}/{table}/search?q={term}.
 * Validates q, lazily connects if *conn is NULL, detects table type,
 * builds + executes a parameterized query, serializes the response,
 * and writes JSON to the response buffer.
 * Connection lifecycle is owned by the caller (handle_client in main.c).
 * @param client_fd The client socket file descriptor.
 * @param conn Borrowed PGconn pointer (may be NULL, never PQfinish'd here).
 * @param database The database config containing the target table.
 * @param raw_table_name The table name, already lower_snake_cased.
 * @param q The decoded search term from the querystring.
 * @param response Output pointer for the response body.
 * @param response_len Output pointer for the response length.
 * @param cookie_header The incoming Cookie header for session auth.
 */
extern void handle_search(int client_fd, PGconn **conn, struct db *database,
                          const char *raw_table_name, const char *q,
                          char **response, size_t *response_len,
                          const char *cookie_header);

#endif
