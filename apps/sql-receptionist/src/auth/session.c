#include "auth/hash.h"
#include "auth/rng.h"
#include "auth/string.h"
#include "postgres.h"
#include <libpq-fe.h>
#include <openssl/evp.h>
#include <string.h>
#include <time.h>

int create_session(char *username, char *token, PGconn *conn) {
  time_t now = time(NULL);

  token[RANDOM_STRING_LENGTH] = '\0'; // temporarily null terminate for libpq
  char *secret = token + RANDOM_STRING_LENGTH + 1;
  char secret_hash_hex[65];
  secret[RANDOM_STRING_LENGTH] = '\0'; // null terminate token

  // generate token
  generate_secure_random_string(token);                            // id
  generate_secure_random_string(token + RANDOM_STRING_LENGTH + 1); // secret

  // hash the secret
  sha_256_hex(secret, RANDOM_STRING_LENGTH, secret_hash_hex);
  secret_hash_hex[64] = '\0';

  PGresult *res =
      PQexecParams(conn,
                   "INSERT INTO sessions (id, user_id, secret_hash) VALUES($1, "
                   "(SELECT id FROM users where USERNAME = $2), $3)",
                   3, NULL, (const char *[]){token, username, secret_hash_hex},
                   NULL, NULL, 0);

  // fix token string
  token[RANDOM_STRING_LENGTH] = '.';

  int success = res != NULL && PQresultStatus(res) == PGRES_COMMAND_OK;
  PQclear(res);

  return success;
}

int validate_token(char *token, char *username, PGconn *conn) {
  const char *const param_values[1] = {token};
  PGresult *res = PQexecParams(
      conn,
      "SELECT sessions.secret_hash, users.username WHERE sessions.id = $1 "
      "INNER JOIN users ON users.id=sessions.user_id",
      1, NULL, param_values, NULL, NULL, 0);

  if (!res)
    return 0;

  if (PQnfields(res) == 0) {
    PQclear(res);
    return 0;
  }

  char secret_hash_hex1[64];
  char secret_hash_hex2[64];
  sha_256_hex(token + RANDOM_STRING_LENGTH + 1, RANDOM_STRING_LENGTH,
              secret_hash_hex1);
  memcpy(secret_hash_hex2, PQgetvalue(res, 0, 0), 64);

  if (!constant_time_string_equality(secret_hash_hex1, secret_hash_hex2, 64)) {
    PQclear(res);
    return 0;
  }

  const char *username_raw = PQgetvalue(res, 0, 1);
  size_t n = strlen(username_raw);

  memcpy(username, username_raw, n);
  username[n] = '\0';

  PQclear(res);
  return 1;
}