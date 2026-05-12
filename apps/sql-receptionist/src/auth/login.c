#include "auth/creds.h"
#include "auth/session.h"
#include "postgres.h"
#include "server/responses.h"
#include <errno.h>
#include <jansson.h>
#include <libpq-fe.h>
#include <string.h>

int login(char *token, const char *username, const char *password,
          PGconn *conn) {
  if (check_creds(username, password, conn) &&
      create_session(username, token, conn)) {
    return 1;
  } else {
    return 0;
  }
}

void handle_login(char **response, size_t *response_len, const char *body) {
  json_error_t parse_status;
  json_t *entry = json_loads(body, 0, &parse_status);
  if (!entry) {
    build_response(400, response, response_len, "",
                   "Failed to parse body JSON.");
    return;
  }

  json_t *username_entry = json_object_get(entry, "username");
  json_t *password_entry = json_object_get(entry, "password");

  if (!username_entry) {
    build_response(400, response, response_len, "", "Missing username.");
    json_decref(entry);
    return;
  } else if (!password_entry) {
    build_response(400, response, response_len, "", "Missing password.");
    json_decref(entry);
    return;
  } else if (!json_is_string(username_entry)) {
    build_response(400, response, response_len, "",
                   "The username should be a string.");
    json_decref(entry);
    return;
  } else if (!json_is_string(password_entry)) {
    build_response(400, response, response_len, "",
                   "The password should be a string.");
    json_decref(entry);
  }

  const char *username = json_string_value(username_entry);
  const char *password = json_string_value(password_entry);
  char token[TOKEN_LENGTH + 1];

  PGconn *conn = connect_db("info");
  if (errno) {
    perror("login");
    build_response(500, response, response_len, "",
                   "An error occurred while attempting to authenticate.");
    PQfinish(conn);
    return;
  }

  if (login(token, username, password, conn)) {
    char cookie_header[TOKEN_COOKIE_HEADER_MAX_LENGTH + 1];
    memcpy(write_cookie_header(cookie_header), token, TOKEN_LENGTH);
    build_response(200, response, response_len, cookie_header,
                   "Login succeeded!");
  } else {
    build_response(403, response, response_len, "", "Invalid credentials.");
  }

  PQfinish(conn);
  json_decref(entry);
}