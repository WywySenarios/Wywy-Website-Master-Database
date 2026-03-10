#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif

struct select_options {
    /**
     * @param table_name the target table to SELECT from
     */
    const char *table_name;
    /**
     * @param order_by_column which column to order by. Must not be NULL.
     */
    const char *order_by_column;
    /**
     * @param order_by_order ASC or DSC.
     */
    const char *order_by_order;
    /**
     * @param schema The item schema to construct a SELECT query around.
     */
    const struct data_column *schema;
    /**
     * @param schema_count The amount of data columns inside of the schema.
     */
    const unsigned int schema_count;
    /**
     * @param limit The upper bound of the number of entries to return. Limit is directly injected into the SELECT query. The query will not be upper bounded if limit is set to a negative integer.
     */
    const int limit;
    /**
     * @param row_offset The row number offset. This number is ignored (not baked into the SQL query) if it is 0.
     */
    const int row_offset;
};

/**
 * Construct a select query based on the given options.
 * This function sets errno to ENOMEM on memory allocation failure.
 * This function does not validate errno beforehand.
 * @TODO add filters
 * @param options The options for the select query. The value is not validated.
 */
extern char *construct_select_query(struct select_options *options);