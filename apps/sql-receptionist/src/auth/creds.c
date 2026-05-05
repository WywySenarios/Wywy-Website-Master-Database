#include "auth/hash.h"
#include "auth/string.h"
#include <argon2.h>
#include <libpq-fe.h>
#include <string.h>

int check_creds(char *username, char *password, PGconn *conn) {
  char phc_string[ARGON2_PHC_SIZE];

  PGresult *res =
      PQexecParams(conn, "SELECT password_hash FROM users WHERE username=$1", 1,
                   NULL, (const char **){}, NULL, NULL, 0);
  if (!res || PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
    PQclear(res);
    return 0;
  }
  memcpy(phc_string, PQgetvalue(res, 0, 0), 64);
  phc_string[64] = '\0';
  PQclear(res);

  return argon2id_verify(phc_string, password, strlen(password)) == ARGON2_OK;
}