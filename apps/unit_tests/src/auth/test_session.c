#include "assert_test.h"
#include "auth/hash.h"
#include "auth/rng.h"
#include "auth/session.h"
#include "logging.h"
#include "postgres.h"
#include <errno.h>
#include <libpq-fe.h>
#include <string.h>

int test_session() {
  // @TODO fix pass/fail
  char token[RANDOM_STRING_LENGTH + 1 + RANDOM_STRING_LENGTH + 1];
  char username[65];

  PGconn *conn = connect_db("info");
  if (errno) {
    perror("test_session_creation DB conn");
    log_critical("test_session_creation DB conn");
    return 0;
  }

  assert_false(
      validate_token(username,
                     "xxxxxxxxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxxxxxxxx", conn),
      "An invalid token was deemed valid by validate_session.");

  char fail_user_password_hash[ARGON2_PHC_SIZE];
  const char *fail_user_param_values[1] = {fail_user_password_hash};
  argon2id_phc("password", 1, fail_user_password_hash);

  PGresult *res =
      PQexecParams(conn,
                   "INSERT INTO users (username, password_hash, access_level, "
                   "tokens_remaining) VALUES ('bad_session_user', $1, 0, 0)",
                   1, NULL, fail_user_param_values, NULL, NULL, 0);
  if (res && PQresultStatus(res) == PGRES_COMMAND_OK) {
    assert_false(
        create_session("bad_session_user", token, conn),
        "F: An invalid session (empty token bucket) creation succeeded.");
  } else {
    puts("F: Valid user INSERT failed ((username, password_hash, access_level, "
         "tokens_remaining) VALUES ('bad_session_user', [password_hash], 0, "
         "-10000)).");
  }
  PQclear(res);

  assert_false(
      create_session("not_admin", token, conn),
      "F: Session creation erroneously succeded for a non-existent user.");

  // START - integration test: valid session creation
  // ensure that the session creation function runs to completion
  int session_OK = create_session("admin", token, conn) == 1;
  assert_true(session_OK, "Session creation did not run to completion.");

  if (!session_OK) {
    PQfinish(conn);
    return 0;
  }

  // ensure that the token has valid text
  for (int i = 0; i < RANDOM_STRING_LENGTH + 1 + RANDOM_STRING_LENGTH; i++) {
    if (token[i] == '\0') {
      puts("\"Successful\" session creation did not produce a valid token.");
      PQfinish(conn);
      return 0;
    }
  }

  // ensure that the related database record exists
  const char *query_placeholders[1] = {token};
  token[RANDOM_STRING_LENGTH] =
      '\0'; // allow libpq to do its thing with string lengths
  res = PQexecParams(conn, "SELECT (secret_hash) FROM sessions WHERE id=$1", 1,
                     NULL, query_placeholders, NULL, NULL, 0);
  if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
    puts("create_session's related database record could not be found.");
    PQfinish(conn);
    return 0;
  }

  // ensure that a correct secret can be verified
  char *secret_hash_hex1 = PQgetvalue(res, 0, 0);

  if (!secret_hash_hex1) {
    puts("Unexpectedly NULL secret hash.");
    PQfinish(conn);
    return 0;
  }

  char secret_hash_hex2[32];
  sha_256_hex(token + RANDOM_STRING_LENGTH + 1, RANDOM_STRING_LENGTH,
              secret_hash_hex2);

  assert_true(
      strcmp(secret_hash_hex1, secret_hash_hex2) == 0,
      "The database secret has does not match with the client secret hash.");
  PQclear(res);

  // test validation
  assert_true(validate_token(username, token, conn),
              "A valid session was not deemed valid by validate_session.");
  assert_true(strcmp(username, "admin") == 0,
              "validate_token did not return the same username that the token "
              "was created for.");

  PQfinish(conn);

  return 1;
}