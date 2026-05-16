#include "auth/hash.h"
#include "auth/rng.h"
#include "auth/string.h"
#include "postgres.h"
#include <libpq-fe.h>
#include <openssl/evp.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int create_session(const char *username, char *token, PGconn *conn) {
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
  PGresult *res = PQexecParams(
      conn,
      "WITH relevant_user AS (UPDATE users SET tokens_remaining = LEAST(1000, "
      "tokens_remaining + EXTRACT(EPOCH FROM (now() - last_seen)) - 1), "
      "last_seen = NOW() WHERE username=$1 RETURNING id) INSERT INTO sessions "
      "(id, user_id, secret_hash) SELECT $2, id, $3 FROM relevant_user",
      3, NULL, (const char *[]){username, token, secret_hash_hex}, NULL, NULL,
      0);

  // fix token string
  token[RANDOM_STRING_LENGTH] = '.';

  int success = res != NULL && PQresultStatus(res) == PGRES_COMMAND_OK &&
                strcmp(PQcmdTuples(res), "0") != 0;
  PQclear(res);

  return success;
}

int validate_token(char *username, const char *token, PGconn *conn) {
  static const int param_formats[1] = {1};
  static const int param_lengths[1] = {RANDOM_STRING_LENGTH};
  PGresult *res =
      PQexecParams(conn,
                   "SELECT sessions.secret_hash, users.username, "
                   "FLOOR(EXTRACT(EPOCH FROM (NOW() - "
                   "sessions.last_seen))/60/60)::INT FROM sessions "
                   "INNER JOIN users ON "
                   "users.id=sessions.user_id WHERE sessions.id=$1",
                   1, NULL, &token, param_lengths, param_formats, 0);

  if (!res || PQntuples(res) == 0) {
    PQclear(res);
    return 0;
  }

  // check age first
  const char *age =
      PQgetvalue(res, 0, 2); // I could definitely use strtol, but I feel lazy
  if (strlen(age) >= 4) {    // expires after 1000 hours, or ~41.6 days
    PQclear(res);
    return 0;
  }

  char secret_hash_hex1[65];
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

  PGresult *last_seen_res = NULL;
  if (strlen(age) > 1) {
    // update last seen after 1 hour
    // it's OK for this to fail and run the next time
    last_seen_res =
        PQexecParams(conn,
                     "UPDATE users SET last_seen=NOW() FROM sessions WHERE "
                     "sessions.id=$1 AND users.id=sessions.user_id",
                     1, NULL, &token, param_lengths, param_formats, 0);
    if (!last_seen_res || PQresultStatus(last_seen_res) != PGRES_COMMAND_OK) {
      goto validate_token_end;
    }
    PQclear(last_seen_res);
    last_seen_res =
        PQexecParams(conn, "UPDATE sessions SET last_seen=NOW() WHERE id=$1", 1,
                     NULL, &token, param_lengths, param_formats, 0);
    if (!last_seen_res || PQresultStatus(last_seen_res) != PGRES_COMMAND_OK) {
      goto validate_token_end;
    }
  }
validate_token_end:
  PQclear(last_seen_res);
  PQclear(res);
  return 1;
}