#include <libpq-fe.h>

/**
 * Executes an SQL query with the given query and connection. @TODO retry when the connection is bad.
 * @param query The query to execute.
 * @param res The output pointer to store the result.
 * @param conn The connection to use.
 * @returns The status of the executed query.
 */
ExecStatusType sql_query(const char *query, PGresult **res, PGconn *conn);

/**
 * Open a new Postgres connection using the given database name. Automatically populates environment variables.
 * Sets errno to ENOMEM if the database name is too large. This generally should not happen so it should be considered a bug if it does.
 * @param dbname
 */
PGconn *connect_db(const char *dbname);