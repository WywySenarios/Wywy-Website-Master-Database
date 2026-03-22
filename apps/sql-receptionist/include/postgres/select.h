#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#include "libpq-fe.h"
#include <stdlib.h>
#define SELECT_DEFAULT_LIMIT 500

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
     * @param id_column Whether or not the ID column is a part of this table.
     */
    const int id_column;
    /**
     * @param primary_tag Whether or not the primary_tag column is a part of this table.
     */
    const int primary_tag;
    /**
     * Whether or not the primary tag column should be transformed to represent the tag names instead of the tag ID.
     * @param transform_tag_names
     */
    const int transform_tag_names;
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
 * Find the expected string size to store a select query.
 * @TODO add filters
 * @param options The options for the select query. The value is not validated.
 */
extern size_t select_query_size(struct select_options *options);

/**
 * Construct a select query based on the given options. Guarentees buffer safety.
 * Sets errno to ENOMEM when the buffer runs out of space.
 * Sets errno to EILSEQ if an snprintf call fails.
 * @TODO add filters
 * @param options The options for the select query. The value is not validated.
 * @param buffer The buffer to write to.
 * @param buffer_size The maximum amount of characters the buffer can store.
 */
extern void construct_select_query(struct select_options *options, char *buffer, size_t buffer_size);

/**
 * Serialize a SELECT query result into JSON. Guarentees buffer safety.
 * Sets errno to ENOMEM when the buffer runs out of space.
 * @param res The PGresult to serialize.
 * @param buffer The buffer to write to.
 * @param buffer_size The maxmimum amount of characters the buffer can store.
 * @returns the length of the string written or -1 on error.
 */
extern int serialize_select_result(const PGresult *res, char *buffer, const size_t buffer_size);