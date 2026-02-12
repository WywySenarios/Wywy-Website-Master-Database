#include "config.h"
#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

/**
 * Attempts to query the database with the given query.
 * This function assumes that the global config has the correct information on
 * the
 * @param dbname A pointer to a sequence of characters representing the database
 * name to connect to. This function does NOT free dbname.
 * @param query A pointer to a sequence of characters representing the query to
 * execute. This function does NOT free query.
 * @param res A double pointer to a PGresult pointer that will be set to the
 * result of the query. This function does NOT free *res.
 * @param conn A double pointer to a PGconn pointer that will be set to the
 * connection used. This function does NOT free *conn. This function creates a
 * connection if *conn is NULL, and assumes that the connection is correct if it
 * is not NULL. As a result, this function alone does NOT guarentee that the
 * connection refers to the given database name.
 * @param config The global config.
 * @return The status of the query as per PQstatus().
 */
ExecStatusType sql_query(char *dbname, char *query, PGresult **res,
                         PGconn **conn, const struct config *global_config) {
  if (getenv("SQL_RECEPTIONIST_LOG_QUERIES") &&
      strcmp(getenv("SQL_RECEPTIONIST_LOG_QUERIES"), "TRUE") == 0)
    printf("Query: %s\n", query);

  if (*conn == NULL) {
    size_t conninfo_size = 1 + strlen("dbname= user= password= host= port=") +
                           strlen(dbname) +
                           strlen(getenv("DATABASE_USERNAME")) +
                           strlen(getenv("DATABASE_PASSWORD")) +
                           strlen(getenv("DATABASE_HOST")) + 5 + 1;
    char *conninfo = malloc(conninfo_size);
    snprintf(conninfo, conninfo_size,
             "dbname=%s user=%s password=%s host=%s port=%s", dbname,
             getenv("DATABASE_USERNAME"), getenv("DATABASE_PASSWORD"),
             getenv("DATABASE_HOST"), getenv("DATABASE_PORT"));

    *conn = PQconnectdb(conninfo);
    free(conninfo);
  }

  if (PQstatus(*conn) != CONNECTION_OK) {
    return PGRES_FATAL_ERROR;
  }

  // Submit & Execute query
  *res = PQexec(*conn, query);
  ExecStatusType status = PQresultStatus(*res);

  return status;
}