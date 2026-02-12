#include <jansson.h>
#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif

extern int construct_validate_query(json_t *entry, struct data_column *schema,
                             unsigned int schema_count, char **query,
                             char *table_name,
                             const char *primary_column_name, const char *duplicate_column_name);