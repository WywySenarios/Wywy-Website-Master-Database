/**
 * Library to validate data.
 */
#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#include <jansson.h>

/**
 * Validate a JSON item with the given column schema.
 * @param item The item to validate.
 * @param column_schema The schema of the column to validate against.
 * @param column_type The (expected) relationship between the given JSON item and the column schema.
 * @returns -1 on failure to check (out of memory, regcomp failure, etc.). Whether or not the JSON item is conformant to the given column schema (T/F).
 */
extern int validate_column(const json_t *item, struct data_column column_schema, enum column_type column_type);