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
  char token[RANDOM_STRING_LENGTH + 1 + RANDOM_STRING_LENGTH + 1];

  PGconn *conn = connect_db("info");
  if (errno) {
    perror("test_session_creation DB conn");
    log_critical("test_session_creation DB conn");
    return 0;
  }

  assert_false(
      validate_token("admin",
                     "xxxxxxxxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxxxxxxxx", conn),
      "An invalid token was deemed valid by validate_session.");

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
  PGresult *res =
      PQexecParams(conn, "SELECT (secret_hash) FROM sessions WHERE id=$1", 1,
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
  assert_true(validate_token("admin", token, conn),
              "A valid session was not deemed valid by validate_session.");

  PQfinish(conn);

  return 1;
}