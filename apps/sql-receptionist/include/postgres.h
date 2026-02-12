#include <libpq-fe.h>
ExecStatusType sql_query(char *dbname, char *query, PGresult **res,
                         PGconn **conn, const struct config *global_config);