#include <libpq-fe.h>
ExecStatusType sql_query(const char *dbname, const char *query, PGresult **res,
                         PGconn **conn);