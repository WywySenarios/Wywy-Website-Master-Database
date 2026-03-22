#include "config.h"
#include <errno.h>
#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONN_INFO_LENGTH 8192

ExecStatusType sql_query(const char *query, PGresult **res, PGconn *conn) {
  if (getenv("SQL_RECEPTIONIST_LOG_QUERIES") &&
      strcmp(getenv("SQL_RECEPTIONIST_LOG_QUERIES"), "TRUE") == 0)
    printf("Query: %s\n", query);

  // Submit & Execute query
  *res = PQexec(conn, query);

  return PQresultStatus(*res);
}

PGconn *connect_db(const char *dbname) {
  char conninfo[MAX_CONN_INFO_LENGTH];

  if (MAX_CONN_INFO_LENGTH <=
      snprintf(conninfo, MAX_CONN_INFO_LENGTH,
               "dbname=%s user=%s password=%s host=%s port=%s", dbname,
               getenv("DATABASE_USERNAME"), getenv("DATABASE_PASSWORD"),
               getenv("DATABASE_HOST"), getenv("DATABASE_PORT"))) {
    errno = ENOMEM;
    return NULL;
  }

  return PQconnectdb(conninfo);
}