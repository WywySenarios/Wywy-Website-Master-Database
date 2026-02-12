/**
 * This application attempts to dynamically execute SQL queries to regular
 * databases. This application should not handle more sensitive databases, and
 * should instead ignore them.
 * @todo create separate file for basic HTTP server functionalities (this file
 * should only handle decision making)
 * @todo create separate ORM file
 */

#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#include "postgres.h"
#include "postgres/insert.h"
#include "server/responses.h"
#include "utils/format_string.h"
#include "utils/http.h"
#include "utils/json/datatype_validation.h"
#include "utils/regex_item.h"
#include "utils/regex_iterator.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <jansson.h>
#include <libpq-fe.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 2523 // @todo make this configurable
#define BUFFER_SIZE 104857600 - 1
#define MAX_ENTRY_SIZE 1048576 - 1
#define AUTH_DB_NAME "auth" // @todo make this configurable
#define MAX_URL_SECTIONS 4  // must be 2 or larger
#define MAX_REGEX_MATCHES 25
#define limit "20"
#define NUM_DATATYPES_KEYS 1
#define MAX_PASSWORD_LENGTH 255

int done;
void handle_sigterm(int signal_num) {
  printf("Received SIGTERM. Exiting now...\n");
  done = 1;
  exit(0);
}

void handle_sigint(int signal_num) {
  printf("Received SIGINT. Exiting now...\n");
  done = 1;
  exit(0);
}

char *admin_creds;

// nice global variables!
static struct config *global_config = NULL;

static struct data_column tags_schema[] = {
    {"entry_id", "int", false, ""},
    {"tag_id", "int", false, ""},
};
static int tags_schema_count = 2;
static struct data_column tag_names_schema[] = {
    {"tag_name", "string", false, ""},
};
static int tag_names_schema_count = 1;
static struct data_column tag_aliases_schema[] = {
    {"alias", "string", false, ""},
    {"tag_id", "int", false, ""},
};
static int tag_aliases_schema_count = 2;
static struct data_column tag_groups_schema[] = {
    {"tag_id", "int", false, ""},
    {"group_name", "string", false, ""},
};
static int tag_groups_schema_count = 2;

/**
 * Attempts to query the database and builds a response based on the query.
 * Expects the query to be a SELECT query. @TODO optimize by finding the columns
 * before-hand
 * @param database_name The target database's name.
 * @param query The query to execute.
 * @param res The response variable to pass into sql_query
 * @param conn The connection to pass into sql_query
 * @param response The response variable to pass into build_response... .
 * @param response_len The response length variable to pass into
 * build_response... .
 */
void generic_select_query_and_respond(char *database_name, char *query,
                                      PGresult **res, PGconn **conn,
                                      char **response, size_t *response_len) {
  ExecStatusType sql_query_status =
      sql_query(database_name, query, res, conn, global_config);
  if (sql_query_status == PGRES_TUPLES_OK ||
      sql_query_status == PGRES_COMMAND_OK) { // if the query is successful,
    // convert the query information into JSON
    char *output = malloc(BUFFER_SIZE);       // @todo be more specific
    char *column_names = malloc(BUFFER_SIZE); // @todo be more specific
    char *output_arrs = malloc(BUFFER_SIZE);  // @todo be more specific
    // catch malloc failure
    if (!output || !column_names || !output_arrs) {
      build_response(500, response, response_len,
                     "Something went wrong when fetching the query results.");
      free(output);
      free(column_names);
      free(output_arrs);
      return;
    }

    column_names[0] = '\0';
    output_arrs[0] = '\0';

    // add in the column names
    for (int col = 0; col < PQnfields(*res); col++) {
      strcat(column_names, "\"");
      strcat(column_names, PQfname(*res, col));
      strcat(column_names, "\",");
    }
    // remove trailing comma
    column_names[strlen(column_names) - 1] = '\0';

    // add in "[...]," for all the arrays
    // @todo optimize
    for (int row = 0; row < PQntuples(*res); row++) {
      char *entry_arr = malloc(MAX_ENTRY_SIZE);
      strcpy(entry_arr, "[");

      for (int col = 0; col < PQnfields(*res); col++) {
        if (!PQgetisnull(*res, row, col)) {
          int requires_quotes = 0;
          // @todo binary search optimization
          switch (PQftype(*res, col)) {
          case 25:
          case 1082:
          case 1083:
          case 1114:
            requires_quotes = 1;
            strcat(entry_arr, "\"");
            break;
          default:
            requires_quotes = 0;
            break;
          }

          strcat(entry_arr, PQgetvalue(*res, row, col));
          if (requires_quotes) {
            strcat(entry_arr, "\"");
          }
        } else
          strcat(entry_arr, "null");
        strcat(entry_arr, ",");
      }
      // remove trailing comma
      entry_arr[strlen(entry_arr) - 1] = ']';

      strcat(entry_arr, ",");

      strcat(output_arrs, entry_arr);
      free(entry_arr);
    }
    // remove the trailing comma
    output_arrs[strlen(output_arrs) - 1] = '\0';

    snprintf(output, BUFFER_SIZE, "{\"columns\":[%s],\"data\":[%s]}",
             column_names, output_arrs);
    build_response(200, response, response_len, output);
    free(column_names);
    free(output_arrs);
    free(output);
  } else {
    build_response_printf(500, response, response_len,
                          strlen(PQresStatus(sql_query_status)) + 2 +
                              strlen(PQerrorMessage(*conn)) + 1,
                          "%s: %s", PQresStatus(sql_query_status),
                          PQerrorMessage(*conn));
  }
}

char *replace_table_name(char *table_name, const char *suffix) {
  size_t table_len = strlen(table_name) + strlen(suffix) + 1;
  table_name = realloc(table_name, strlen(table_name) + strlen(suffix) + 1);
  memcpy(table_name + strlen(table_name), suffix, strlen(suffix) + 1);
  return table_name;
}

/**
 * Understand the client's request and decide what action to take based on the
 * request.
 */
void *handle_client(void *arg) {
  // variables that are always used:
  int client_fd = *((int *)arg);
  char *buffer = malloc(BUFFER_SIZE * sizeof(char));
  char *response = NULL;
  size_t response_len = 0;

  // variables that are generally useful:
  char *method = NULL;
  char *headers = NULL;
  char *url = NULL;
  char *url_segments[MAX_URL_SECTIONS];
  for (int i = 0; i < MAX_URL_SECTIONS; i++)
    url_segments[i] = NULL;
  regmatch_t *matches = NULL;
  struct regex_iterator *url_regex = NULL;
  char *database_name = NULL;
  struct db *database = NULL;
  // the parent table name
  char *table_name = NULL;
  struct table *table = NULL;
  struct regex_iterator *querystring_regex = NULL;
  PGconn *conn = NULL;
  PGresult *res = NULL;
  char *query = NULL;

  // check for failed memory allocation
  if (!buffer) {
    goto end;
  }

  // receive request data from client and store into buffer
  ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

  // printf("%s\n", buffer);

  if (bytes_received <= 0) {
    build_response(400, &response, &response_len, "No request data received.");
    goto end;
  }

  buffer[bytes_received] = '\0';
  char *body = strstr(buffer, "\r\n\r\n");
  if (body)
    body += 4;

  // @warning HTTP/1 not matching?
  // ^([A-Z]+) /([^ ]*) HTTP/[12]\\.[0-9]
  matches = query_regex("^([A-Z]+) /([^ ]*) HTTP/[12]\\.[0-9].*(\r?\n\r?\n)", 3,
                        REG_EXTENDED, 0, buffer);

  // catch malloc failure, etc.
  if (!matches) {
    build_response(500, &response, &response_len,
                   "Something unexpected happened.");
    goto end;
  }

  if (!regmatch_has_match(matches, 1) || !regmatch_has_match(matches, 2)) {
    build_response(400, &response, &response_len, "Bad HTTP request.");
    goto end;
  }

  // Extract the method
  method = regmatch_get_match(matches, 1, buffer);

  if (!method) {
    build_response(500, &response, &response_len,
                   "Something unexpected happened.");
    goto end;
  }

  // immediately check for OPTIONS requests
  if (strcmp(method, "OPTIONS") == 0) {
    build_response_default(204, &response, &response_len);
    goto end;
  }

  // Extract the headers @TODO this needs to be several times more efficient
  int headers_len = matches[3].rm_so;
  headers = malloc(headers_len + 1);
  if (!headers) {
    build_response(500, &response, &response_len,
                   "Something unexpected happened while extracting headers.");
    goto end;
  }

  memcpy(headers, buffer, headers_len);
  headers[headers_len] = '\0';

  // Check for the correct origin
  size_t main_origin_size = strlen("Origin: /?\n") + strlen(getenv("MAIN_URL"));
  char *main_origin_check = malloc(main_origin_size);
  snprintf(main_origin_check, main_origin_size, "Origin: %s/?\n",
           getenv("MAIN_URL"));
  int origin_check_res =
      regex_check(main_origin_check, 0, REG_EXTENDED, 0, headers);

  size_t cache_origin_size =
      strlen("Origin: /?\n") + strlen(getenv("CACHE_URL"));
  char *cache_origin_check = malloc(cache_origin_size);
  snprintf(cache_origin_check, cache_origin_size, "Origin: %s/?\n",
           getenv("CACHE_URL"));
  int cache_check_res =
      regex_check(cache_origin_check, 0, REG_EXTENDED, 0, headers);
  free(main_origin_check);
  free(cache_origin_check);
  if (origin_check_res != 1 && cache_check_res != 1) {
    build_response(400, &response, &response_len,
                   "Bad origin. Compromised browser?");
    goto end;
  }

  // @todo special/reserved URLs

  // extract URL from request and decode URL
  int url_len = matches[2].rm_eo - matches[2].rm_so;
  char *encoded_url = malloc(url_len + 1);
  strncpy(encoded_url, buffer + matches[2].rm_so,
          url_len); // @todo directly decode buffer
  encoded_url[url_len] = '\0';
  url = url_decode(encoded_url);
  free(encoded_url);

  url_regex = create_regex_iterator("([^/^?]+)[/?]?", 1, REG_EXTENDED);

  regex_iterator_load_target(url_regex, url);

  // START - check URL

  // look for as many url sections as possible
  for (int i = 0; i < MAX_URL_SECTIONS; i++) {
    if (regex_iterator_match(url_regex, 0) != 0)
      break;
    url_segments[i] = regex_iterator_get_match(url_regex, 1);
    if (!url_segments[i]) {
      build_response(500, &response, &response_len,
                     "Memory allocation failed.");
      perror("URL section malloc failure");
      goto end;
    }
    regex_iterator_advance_cur(url_regex);
  }
  // check if the user wants to authenticate
  // @TODO check for CSRF

  // @TODO auth for non-admin users
  if (url_segments[0] && strcmp(url_segments[0], "auth") == 0) {
    if (!body) {
      build_response(400, &response, &response_len,
                     "Failed to parse body. (Invalid/empty?)");
      goto end;
    }
    if (strcmp(body, admin_creds) == 0) {
      // @TODO do not hardcode
      response_len = strlen("HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Access-Control-Allow-Origin: \r\n"
                            "Access-Control-Allow-Headers: Content-Type\r\n"
                            "Access-Control-Allow-Credentials: true\r\n"
                            "Set-Cookie: username=admin; Max-Age=\r\n"
                            "Set-Cookie: password=; Max-Age=\r\n"
                            "Connection: close\r\n"
                            "\r\n") +
                     strlen(getenv("AUTH_COOKIE_MAX_AGE")) +
                     strlen(getenv("AUTH_COOKIE_MAX_AGE")) + strlen(body) +
                     strlen(getenv("MAIN_URL"));
      if (getenv("SQL_RECEPTIONIST_LOG_RESPONSES") &&
          strcmp(getenv("SQL_RECEPTIONIST_LOG_RESPONSES"), "TRUE") == 0)
        printf("Constructing 200 OK response: ---[Authentication]---\n\n");
      response = malloc(response_len + 1);
      snprintf(response, response_len + 1,
               "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/plain\r\n"
               "Access-Control-Allow-Origin: %s\r\n"
               "Access-Control-Allow-Headers: Content-Type\r\n"
               "Access-Control-Allow-Credentials: true\r\n"
               "Set-Cookie: username=admin; Max-Age=%s\r\n"
               "Set-Cookie: password=%s; Max-Age=%s\r\n"
               "Connection: close\r\n"
               "\r\n",
               getenv("MAIN_URL"), getenv("AUTH_COOKIE_MAX_AGE"), body,
               getenv("AUTH_COOKIE_MAX_AGE"));
      goto end;
    } else {
      build_response(403, &response, &response_len, "Invalid credentials.");
      goto end;
    }
  }
  // @todo non-admin cookies

  // @todo tokens
  regex_t raw_cookie_regex;
  regcomp(&raw_cookie_regex, "Cookie: (.*)[\r\n]", REG_EXTENDED);

  regmatch_t raw_cookie_matches[1 + 1];
  // auth fails because request has no cookies
  if (regexec(&raw_cookie_regex, buffer, 1 + 1, raw_cookie_matches, 0) ==
      REG_NOMATCH) {
    regfree(&raw_cookie_regex);
    build_response(403, &response, &response_len,
                   "Authentication failed. No username or password provided.");
    goto end;
  }

  int raw_cookies_len =
      raw_cookie_matches[1].rm_eo - raw_cookie_matches[1].rm_so;
  char *raw_cookies = malloc(raw_cookies_len + 1);
  strncpy(raw_cookies, buffer + raw_cookie_matches[1].rm_so, raw_cookies_len);
  raw_cookies[raw_cookies_len] = '\0';
  regfree(&raw_cookie_regex);

  bool admin_username = false;
  bool admin_password = false;

  regex_t cookie_regex;
  regcomp(&cookie_regex, "[ ]*([^= ;]+)[ ]*=[ ]*([^;\r\n]+)", REG_EXTENDED);

  regmatch_t cookie_matches[2 + 1];
  char *cursor = raw_cookies;
  while (regexec(&cookie_regex, cursor, 2 + 1, cookie_matches, 0) == 0) {
    int key_len = cookie_matches[1].rm_eo - cookie_matches[1].rm_so;
    char *key = malloc(key_len + 1);
    strncpy(key, cursor + cookie_matches[1].rm_so, key_len);
    key[key_len] = '\0';

    int value_len = cookie_matches[2].rm_eo - cookie_matches[2].rm_so;
    char *value = malloc(value_len + 1);
    strncpy(value, cursor + cookie_matches[2].rm_so, value_len);
    value[value_len] = '\0';

    if (strcmp(key, "username") == 0)
      admin_username = strcmp(value, "admin") == 0;
    else if (strcmp(key, "password") == 0)
      admin_password = strcmp(value, admin_creds) == 0;

    free(value);
    free(key);

    cursor += cookie_matches[0].rm_eo;

    // Skip any leftover semicolons or spaces
    while (*cursor == ';' || *cursor == ' ')
      cursor++;
  }

  regfree(&cookie_regex);
  free(raw_cookies);

  // require authentication for all other endpoints
  if (!(admin_username && admin_password)) {
    build_response(403, &response, &response_len, "Authentication failed.");
    goto end;
  }

  // get database name
  database_name = url_segments[0];

  // ensure that there is a target database
  if (!database_name) {
    build_response(400, &response, &response_len,
                   "Database name was not supplied.");
    goto end;
  }

  to_lower_snake_case(database_name);

  for (unsigned int i = 0; i < global_config->dbs_count; i++) {
    if (strcmp(global_config->dbs[i].db_name, database_name) == 0) {
      database = &global_config->dbs[i];
      break;
    }
  }

  // ensure that there is a target database
  if (!database) {
    build_response(400, &response, &response_len, "Database not found.");
    goto end;
  }

  table_name = url_segments[1];

  // ensure that there is a target table
  if (!table_name) {
    build_response(400, &response, &response_len, "Table name not supplied.");
    goto end;
  }

  to_lower_snake_case(table_name);

  for (unsigned int i = 0; i < database->tables_count; i++) {
    if (strcmp(database->tables[i].table_name, table_name) == 0) {
      table = &database->tables[i];
    }
  }

  // ensure that there is a target table
  if (!table) {
    build_response(400, &response, &response_len, "Table not found.");
    goto end;
  }
  // END - check URL

  // @todo handle memory?
  // handle querystring
  char *querystring = NULL;
  char *path = url;
  char *qmark = strchr(url, '?');
  if (qmark) {
    *qmark = '\0';           // terminate path at the first '?'
    querystring = qmark + 1; // everything after is the querystring
  }

  // decide what to do
  // first ensure that the method is uppercase
  // @todo verify if this is really needed
  for (int i = 0; method[i] != '\0'; i++) {
    method[i] = toupper((unsigned char)method[i]);
  }

  // search for the relevant table & database

  if (strcmp(method, "GET") == 0) {
    // check if the database & table may be accessed freely
    if (table->read) {
      // try to access the database and query

      // @todo allow-list input validation
      // @todo still vulnerable to changing the config

      if (!url_segments[2]) {
        goto regular_table;
      }

      /*
       * Special endpoints:
       * tag_names & tag_aliases are restricted to SELECT * FROM ...;
       */
      if (strcmp(url_segments[2], "tag_names") == 0) {
        // check if tagging is enabled
        if (!table->tagging) {
          build_response_printf(400, &response, &response_len,
                                strlen("Tagging is not enabled on table \"\""),
                                "Tagging is not enabled on table \"%s\"",
                                table_name);
          goto end;
        }

        size_t query_len =
            strlen("SELECT * FROM _tag_names;") + strlen(table_name);
        query = malloc(query_len + 1);
        snprintf(query, query_len, "SELECT * FROM %s_tag_names;", table_name);
        generic_select_query_and_respond(database_name, query, &res, &conn,
                                         &response, &response_len);
      } else if (strcmp(url_segments[2], "tag_aliases") == 0) {
        // check if tagging is enabled
        if (!table->tagging) {
          build_response_printf(400, &response, &response_len,
                                strlen("Tagging is not enabled on table \"\""),
                                "Tagging is not enabled on table \"%s\"",
                                table_name);
          goto end;
        }

        size_t query_len =
            strlen("SELECT * FROM _tag_aliases;") + strlen(table_name);
        query = malloc(query_len + 1);
        snprintf(query, query_len, "SELECT * FROM %s_tag_aliases;", table_name);
        generic_select_query_and_respond(database_name, query, &res, &conn,
                                         &response, &response_len);
        goto end;
      } else {
        build_response(400, &response, &response_len, "Bad URL.");
      }
      goto end;
    regular_table: // @todo catch tags & tag_groups
      // REQUIRES querystring to run
      if (querystring == NULL) {
        build_response(400, &response, &response_len,
                       "The querystring cannot be empty. It needs to specify "
                       "SELECT options.");
        goto end;
      }

      // slash all &'s separate, and the first = sign after the start of the
      // string or the last &
      querystring_regex =
          create_regex_iterator("[&]?([^=]+)=([^&]+)", 2, REG_EXTENDED);
      if (!querystring_regex) {
        build_response(500, &response, &response_len,
                       "Something went wrong while parsing the querystring.");
        goto end;
      }
      regex_iterator_load_target(querystring_regex, querystring);

      // many of these need to be non-null:
      char *select = NULL;
      char *order_by = NULL;
      char *min = NULL;
      int min_type = -1; // 1 for inclusive, 0 for exclusive
      char *max = NULL;
      int max_type = -1; // 1 for inclusive, 0 for exclusive
      char *start_index = NULL;

      // read every querystring value
      // store every single valid key-value pair.
      char *key = NULL;
      char *value = NULL;
      int valid = 0;
      while (regex_iterator_match(querystring_regex, 0) == 0) {
        key = regex_iterator_get_match(querystring_regex, 1);
        value = regex_iterator_get_match(querystring_regex, 2);
        valid = 0;

        // what type is it?
        if (strcmp(key, "SELECT") == 0) {
          if (strcmp(value, "*") == 0) {
            select = "*";
          } else {
            for (unsigned i = 0; i < table->schema_count; i++) {
              if (strcmp(value, (*table).schema[i].name)) {
                select = value;
                valid = 1;
                break;
              }
            }
          }
        } else if (strcmp(key, "ORDER_BY") == 0) {
          if (strcmp(value, "ASC") == 0) {
            order_by = "ASC";
            valid = 1;
          } else if (strcmp(value, "DSC") == 0) {
            order_by = "DSC";
            valid = 1;
          }
        } else if (strcmp(key, "MIN_INCLUSIVE") == 0) {
          valid = regex_check("^[0-9]$", 0, REG_EXTENDED, 0, value);
          if (valid == 1) {
            if (min_type == -1) {
              min = value;
              min_type = 1;
            } else {
              valid = 0;
            }
          }
        } else if (strcmp(key, "MAX_INCLUSIVE") == 0) {
          valid = regex_check("^[0-9]$", 0, REG_EXTENDED, 0, value);
          if (valid == 1) {
            if (max_type == -1) {
              max = value;
              max_type = 1;
            } else {
              valid = 0;
            }
          }
        } else if (strcmp(key, "MIN_EXCLUSIVE") == 0) {
          valid = regex_check("^[0-9]$", 0, REG_EXTENDED, 0, value);
          if (valid == 1) {
            if (min_type == -1) {
              min = value;
              min_type = 0;
            } else {
              valid = 0;
            }
          }
        } else if (strcmp(key, "MAX_EXCLUSIVE") == 0) {
          valid = regex_check("^[0-9]$", 0, REG_EXTENDED, 0, value);
          if (valid == 1) {
            if (max_type == -1) {
              max = value;
              max_type = 0;
            } else {
              valid = 0;
            }
          }
        } else {
          build_response_printf(400, &response, &response_len,
                                strlen("Invalid querystring key: \"\".") +
                                    strlen(key),
                                "Invalid querystring key: \"%s\".", key);
          free(key);
          free(value);
          goto end;
        }

        // } else if (strcmp(item_name, "INDEX_BY")) {
        switch (valid) {
        case 0:
          build_response_printf(400, &response, &response_len,
                                strlen("Key value pair \"\", \"\" is "
                                       "invalid.") +
                                    strlen(key) + strlen(value),
                                "Key value pair \"%s\", "
                                "\"%s\" is invalid.",
                                key, value);
          free(key);
          free(value);
          goto end;
        case 1:
          free(key);
          free(value);
          break;
        default:
          build_response_printf(
              400, &response, &response_len,
              strlen("Something went wrong when checking querystring key "
                     "\"\".") +
                  strlen(key),
              "Something went wrong when checking querystring key \"%s\".",
              key);
          free(key);
          free(value);
          goto end;
        }
      }

      // are the mandatory request params valid? We need something to select and
      // an order to sort it by.
      if (select && order_by) {
        size_t query_len = strlen("SELECT \nFROM \nORDER BY id \nLIMIT;") +
                           strlen(select) + strlen(table_name) +
                           strlen(order_by) + strlen(limit);
        query = malloc(query_len + 1);

        // decide the SQL query:
        snprintf(query, query_len,
                 "SELECT %s\nFROM %s\nORDER BY id %s\nLIMIT %s;", select,
                 table_name, order_by, limit);
        // add in the optional request params
        // @todo min/max
        // add in the last thing

        // attempt to query the database
        generic_select_query_and_respond(database_name, query, &res, &conn,
                                         &response, &response_len);
      } else {
        build_response(400, &response, &response_len,
                       "SELECT queries need a valid target (SELECT) and a "
                       "valid ordering (ORDER BY)");
      }

      // free(select);
      // free(order_by);
      if (min) {
        free(min);
      }
      if (max) {
        free(max);
      }
    } else {
      // user does not have read access to the respective table
      build_response_printf(403, &response, &response_len,
                            strlen("Read is not enabled on table .") +
                                strlen(table->table_name),
                            "Read is not enabled on "
                            "table %s.",
                            table->table_name);
    }
  } else if (strcmp(method, "POST") == 0) {
    // check if the database & table can be written to freely
    if (table->write) {
      // verify the schema
      if (!body) {
        build_response(400, &response, &response_len,
                       "Failed to parse body. (Invalid/empty?)");
        goto end;
      }

      if (getenv("SQL_RECEPTIONIST_LOG_INPUT") &&
          strcmp(getenv("SQL_RECEPTIONIST_LOG_INPUT"), "TRUE") == 0)
        printf("Body: %s\n\n", body);

      json_t *entry;
      json_error_t entry_error;
      entry = json_loads(body, 0, &entry_error);

      if (!entry) {
        // @TODO respond with line number, etc.
        build_response_printf(400, &response, &response_len,
                              strlen(entry_error.text), "%s", entry_error.text);
        goto schema_mismatch_end;
      }

      char *target_type = url_segments[2];
      if (!target_type) {
        build_response(400, &response, &response_len,
                       "No target table type supplied.");
        goto schema_mismatch_end;
      }

      struct data_column *schema = NULL;
      int schema_count = -1;
      const char *primary_column_name = "id";
      const char *duplicate_column_name = "id";
      if (strcmp(target_type, "data") == 0) {
        // no additional checks needed
        schema = table->schema;
        schema_count = table->schema_count;
      } else if (strcmp(target_type, "descriptors") == 0) {
        if (!table->descriptors) {
          build_response_printf(400, &response, &response_len,
                                strlen("Table does not have descriptors.") +
                                    strlen(table_name),
                                "Table %s does not "
                                "have descriptors.",
                                table_name);
          goto schema_mismatch_end;
        }

        char *descriptor_name = url_segments[3];

        // look for the respective descriptor
        for (int i = 0; i < table->descriptors_count; i++) {
          if (str_cci_cmp(table->descriptors[i].name, descriptor_name) == 0) {
            schema = table->descriptors[i].schema;
            schema_count = table->descriptors[i].schema_count;
            break;
          }
        }

        if (!schema || schema_count == -1) {
          build_response(400, &response, &response_len,
                         "Descriptor schema not found.");
          goto schema_mismatch_end;
        }

        // update target table name
        int old_table_name_len = strlen(table_name);
        url_segments[1] = table_name =
            realloc(table_name, old_table_name_len + strlen(descriptor_name) +
                                    strlen("__descriptors") + 1);
        snprintf(table_name + old_table_name_len,
                 strlen(descriptor_name) + strlen("__descriptors") + 1,
                 "_%s_descriptors", descriptor_name);
        to_lower_snake_case(table_name);
      } else if (strcmp(target_type, "tags") == 0) {
        schema = tags_schema;
        schema_count = tags_schema_count;

        // update target table name
        url_segments[1] = table_name = replace_table_name(table_name, "_tags");
      } else if (strcmp(target_type, "tag_names") == 0) {
        schema = tag_names_schema;
        schema_count = tag_names_schema_count;

        duplicate_column_name = "tag_name";

        // update target table name
        url_segments[1] = table_name =
            replace_table_name(table_name, "_tag_names");
      } else if (strcmp(target_type, "tag_aliases") == 0) {
        schema = tag_aliases_schema;
        schema_count = tag_aliases_schema_count;

        primary_column_name = "alias";
        duplicate_column_name = "alias";

        // update target table name
        url_segments[1] = table_name =
            replace_table_name(table_name, "_tag_aliases");
      } else if (strcmp(target_type, "tag_groups") == 0) {
        schema = tag_groups_schema;
        schema_count = tag_groups_schema_count;

        // update target table name
        url_segments[1] = table_name =
            replace_table_name(table_name, "_tag_groups");
      } else {
        build_response(400, &response, &response_len,
                       "Invalid target table type.");
        goto schema_mismatch_end;
      }

      if (!schema || schema_count == -1) {
        build_response(
            500, &response, &response_len,
            "Bad target table schema. Contact website maintainer for a fix.");
        goto schema_mismatch_end;
      }

      switch (construct_validate_query(entry, schema, schema_count, &query,
                                       table_name, primary_column_name,
                                       duplicate_column_name)) {
      case 0:
        build_response(400, &response, &response_len,
                       "The given entry does not "
                       "conform to the schema.");
        goto schema_mismatch_end;
      case 1:
        break;
      default:
        build_response(
            500, &response, &response_len,
            "Something went wrong while checking your entry with the schema.");
        goto schema_mismatch_end;
      }

      ExecStatusType sql_query_status =
          sql_query(database_name, query, &res, &conn, global_config);
      if (sql_query_status != PGRES_COMMAND_OK &&
          sql_query_status != PGRES_TUPLES_OK) {
        build_response_printf(500, &response, &response_len,
                              strlen(PQresStatus(sql_query_status)) + 2 +
                                  strlen(PQerrorMessage(conn)) + 1,
                              "%s: %s", PQresStatus(sql_query_status),
                              PQerrorMessage(conn));
      } else {
        build_response(200, &response, &response_len, PQgetvalue(res, 0, 0));
      }

    schema_mismatch_end:
    post_bad_input_end:
    bad_body_end:
      // free json object??? how ???
      // FIX LATER: causes memory & multithreading issues??
      json_decref(entry);
    } else {
      // user does not have write access to the respective table
      build_response_printf(
          403, &response, &response_len,
          strlen("Write is not enabled on table .") + strlen(table->table_name),
          "Write is not enabled on table %s.", table->table_name);
    }
  } else {
    build_response(400, &response, &response_len, "Unsupported HTTP method.");
  }

end:
  // send HTTP response to client
  // @TODO determine if NULL checks are necessary
  if (response && *response && response_len)
    send(client_fd, *&response, *&response_len, 0);
  close(client_fd);

  free(buffer);
  free(response);
  free(matches);
  free(method);
  free(url);
  free(headers);
  for (int i = 0; i < MAX_URL_SECTIONS; i++) {
    free(url_segments[i]);
  }
  free_regex_iterator(url_regex);
  free_regex_iterator(querystring_regex);
  PQfinish(conn);
  PQclear(res);
  free(query);
  return NULL;
}

int main(int argc, char const *argv[]) {
  // exit if environment variables are missing
  if (!getenv("DATABASE_HOST")) {
    fprintf(stderr, "Could not find environment variable DATABASE_HOST");
    exit(EXIT_FAILURE);
  }

  if (!getenv("DATABASE_PORT")) {
    fprintf(stderr, "Could not find environment variable DATABASE_PORT.");
    exit(EXIT_FAILURE);
  }
  if (!getenv("DATABASE_USERNAME")) {
    fprintf(stderr, "Could not find environment variable DATABASE_USERNAME.");
    exit(EXIT_FAILURE);
  }
  if (!getenv("DATABASE_PASSWORD")) {
    fprintf(stderr, "Could not find environment variable DATABASE_PASSWORD.");
    exit(EXIT_FAILURE);
  }
  if (!getenv("MAIN_URL")) {
    fprintf(stderr, "Could not find environment variable MAIN_URL");
    exit(EXIT_FAILURE);
  }

  // attempt to read admin password
  admin_creds = malloc(MAX_PASSWORD_LENGTH + 1);
  FILE *admin_secret = fopen("/run/secrets/admin", "r");

  if (!admin_secret) {
    printf("Could not find admin password secret.");
    exit(EXIT_FAILURE);
  }

  fgets(admin_creds, MAX_PASSWORD_LENGTH + 1, admin_secret);

  fclose(admin_secret);

  // setup for SIGTERM
  done = 0;
  signal(SIGTERM, handle_sigterm);
  signal(SIGINT, handle_sigint);

  // populate global variables
  load_config(&global_config);
  if (global_config == NULL) {
    fprintf(stderr, "Failed to load configuration.\n");
    return EXIT_FAILURE;
  } else {
    printf("Successfully loaded config:\n");
    printf(" * Postgres Settings:\n");
    printf("   - Host: %s\n", getenv("DATABASE_HOST"));
    printf("   - Port: %s\n", getenv("DATABASE_PORT"));
    printf("   - User: %s\n", getenv("DATABASE_USERNAME"));
    printf("Recognized %u databases:\n", global_config->dbs_count);
    for (unsigned int i = 0; i < global_config->dbs_count; i++) {
      // transform all database names into lower snake case
      to_lower_snake_case(global_config->dbs[i].db_name);

      printf(" * %s:\n", global_config->dbs[i].db_name);
      printf("   - Name: %s\n", global_config->dbs[i].db_name);
      for (unsigned int j = 0; j < global_config->dbs[i].tables_count; j++) {
        // transform all table names into lower snake case
        to_lower_snake_case(global_config->dbs[i].tables[j].table_name);
        printf("     - Table %s:\n",
               global_config->dbs[i].tables[j].table_name);
        printf("       + Read: %s\n",
               global_config->dbs[i].tables[j].read ? "true" : "false");
        printf("       + Write: %s\n",
               global_config->dbs[i].tables[j].write ? "true" : "false");
      }
    }
  }

  // Set up the server
  int server_fd;
  size_t valread;
  struct sockaddr_in address;
  int opt = 1;
  socklen_t addrlen = sizeof(address);

  // Creating socket file descriptior
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Attach socket to the given port
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // continue attaching socket to the given port
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  printf("Listening on Port %u.\n", PORT);

  while (!done) {
    // client info
    int client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);

    // accept client connection?
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    // create a new thread to handle client request
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, &client_fd) == 0) {
      pthread_detach(thread_id);
    } else {
      // @todo send a nice error msg
      perror("thread create");
      close(client_fd);
    }
  }

  // close the listening socket
  close(server_fd);
  return 0;
}