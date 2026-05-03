#include "auth/hash.h"
#include "auth/rng.h"
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