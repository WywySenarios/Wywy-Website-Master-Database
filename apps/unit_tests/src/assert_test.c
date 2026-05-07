#include "logging.h"
#include "postgres.h"
#include <errno.h>
#include <libpq-fe.h>
#include <regex.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int assert_true(int value, const char *message) {
  if (value) {
    return 1;
  } else {
    log_error(message);
    return 0;
  }
}

int assert_false(int value, const char *message) {
  if (value) {
    log_error(message);
    return 0;
  } else {
    return 1;
  }
}

int assert_database_connection(PGconn **conn, const char *database_name) {
  *conn = connect_db(database_name);
  if (errno) {
    perror("DB conn");
    log_critical_printf("Database connection to %s failed.\n");
    errno = 0;
    return 0;
  }
  return 1;
}

int assert_file_readable(char *buffer, size_t n, const char *filepath,
                         const char *mode) {
  FILE *f = fopen(filepath, mode);

  if (errno) {
    perror("Server-side file read");
    log_critical_printf("File read to \"%s\" unexpectedly failed.", filepath);
    errno = 0;
    return 0;
  }

  fgets(buffer, n, f);

  fclose(f);
  return 1;
}

int assert_response_body(char *response, const char *body,
                         const char *message) {
  regex_t regex;
  if (regcomp(&regex, "\r\n\r\n(.*)", REG_EXTENDED) != 0) {
    puts("E: regcomp failed in assert_response_body");
    return 0;
  }
  regmatch_t matches[2];

  if (regexec(&regex, response, 2, matches, 0) == REG_NOMATCH ||
      matches[1].rm_eo == -1) {
    puts(message);
    regfree(&regex);
    return 0;
  } else {
    regfree(&regex);
    return strcmp(response + matches[1].rm_eo, body) == 0;
  }
}