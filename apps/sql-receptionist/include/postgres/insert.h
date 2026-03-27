#include <libpq-fe.h>
#include <jansson.h>
#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#define MAX_SQL_RETURN_LENGTH 101
#define ERROR_BUFFER_SIZE 251

struct insert_options {
    /**
     * @param table_name The name of the table to INSERT INTO.
     */
    const char *table_name;
    /**
     * @param schema The column schema of the table to INSERT INTO.
     */
    const struct data_column *schema;
    /**
     * @param schema_count The number of columns inside the table to INSERT INTO.
     */
    unsigned int schema_count;
    /**
     * @TODO implement this
     * @param primary_tag Whether or not the primary_tag should be included in the INSERT query.
     */
    int primary_tag;
    /**
     * @param primary_column_name The name of the PRIMARY column.
     */
    const char *primary_column_name;
    /**
     * @param duplicate_column_name The name of the column that may be duplicated. Should not be NULL.
     */
    const char *duplicate_column_name;
};

/**
 * Attempts to INSERT INTO the given table with the given data.
 * Writes an error message to the error buffer on failure.
 * Assumes errno has been reset.
 * errno will be set to EILSEQ if the input entry & options contain invalid characters.
 * errno will be set to ENOMEM if the input entry & options exceed the maximum query size limit.
 * errno will be set to EDOM if the code is bugged.
 * The code can be bugged because the input entry has a key with a valid column name and column prefix but can't find a column type (only happens when the code is bugged).
 * The code can be bugged because of an invalid regex pattern.
 * errno will not be set if validate_and_insert_into returns 1.
 * @param options The data to insert.
 * @param res Query result output pointer. Will be NULL if the query does not run. This pointer must be freed afterwards.
 * @param conn Connection pointer. The connection must be closed afterwards.
 * @param error_buffer A buffer with at least ERROR_BUFFER_SIZE of size to write an error message to. The error_buffer will contain the NULL terminator as the first charcater if the query ran successfully.
 * @returns Whether or not the query ran (i.e. whether or not the input data was valid).
 */
extern int validate_and_insert_into(struct insert_options *options, json_t *entry, PGresult **res, PGconn *conn, char *error_buffer);