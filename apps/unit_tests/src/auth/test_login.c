#include "assert_test.h"
#include "auth/login.h"
#include "auth/session.h"
#include "libpq-fe.h"
#include "test_case.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

TEST_SUITE(login) {
  int pass = 1;

  char token[TOKEN_LENGTH + 1];
  char admin_password[257];
  PGconn *conn;

  if (!assert_true(
          assert_file_readable(admin_password, 257, "/run/secrets/admin", "r"),
          "F: Admin password not available.\n")) {
    PQfinish(conn);
    return 0;
  }

  if (!assert_true(assert_database_connection(&conn, "info"),
                   "E: Database connection failed.\n")) {
    PQfinish(conn);
    return 0;
  }

  assert_true(login(token, "admin", admin_password, conn),
              "F: login with valid credentials failed.\n");
  if (*admin_password == '\0') {
    admin_password[0] = 'a';
    admin_password[1] = '\0';
  } else {
    *admin_password = '\0';
  }
  assert_false(login(token, "admin", admin_password, conn),
               "F: login with invalid credentials succeeded.");

  PQfinish(conn);
  return pass;
}

// START - handle_login
#define assert_handle_login(body, expected_status, assert_response_message,    \
                            expected_body, assert_body_message)                \
  char *response = NULL;                                                       \
  size_t response_len = 0;                                                     \
  int pass = 1;                                                                \
  handle_login(&response, &response_len, body);                                \
  pass &= assert_response_status(response, expected_status,                    \
                                 assert_response_message);                     \
  pass &= assert_response_body(response, expected_body, assert_body_message);  \
  free(response);                                                              \
  return pass;

TEST_CASE(handle_login_empty_body){
    assert_handle_login("", 400,
                        "Invalid login (empty body) did not return status 400.",
                        "Failed to parse body JSON.",
                        "Invalid login (empty body) response body was not "
                        "\"Failed to parse body JSON.\".")}

TEST_CASE(handle_login_missing_password){assert_handle_login(
    "{\"username\":\"admin\"}", 400,
    "F: Invalid login (missing password) did not return status 400.",
    "Missing password.",
    "F: Invalid login (missing password) response body was not \"Missing "
    "password.\"."
    "body.")}

TEST_CASE(handle_login_missing_username){assert_handle_login(
    "{\"password\":\"xx\"}", 400,
    "F: Invalid login (missing username) did not return status 400.",
    "Missing username.",
    "F: Invalid login (missing username) response body was not \"Missing "
    "username.\"."
    "body.")}

TEST_CASE(handle_login_valid_login) {
  char admin_password[257];

  if (!assert_true(
          assert_file_readable(admin_password, 257, "/run/secrets/admin", "r"),
          "E: Admin password not available.\n")) {
    return 0;
  }

  char body[36 + 256 + 1];
  snprintf(body, 36 + 256, "{\"username\":\"admin\",\"password\":\"%s\"}",
           admin_password);
  if (errno)
    return 0;

  assert_handle_login(
      body, 200, "F: Valid login did not return status 200.",
      "Login succeeded!",
      "F: Valid login response body is not \"Login succeeded!\".")
}

TEST_SUITE(handle_login) {
  int pass = 1;
  RUN_TEST_CASE(handle_login_valid_login)
  RUN_TEST_CASE(handle_login_empty_body)
  RUN_TEST_CASE(handle_login_missing_password)
  RUN_TEST_CASE(handle_login_missing_username)

  return pass;
}
// END - handle_login