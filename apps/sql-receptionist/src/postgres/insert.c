#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#include "postgres/schema.h"
#include "utils/format_string.h"
#include "utils/json/json_conversion.h"
#include "utils/regex_item.h"
#include <jansson.h>
#include <regex.h>
#include <string.h>

/**
 * Goes through the entry and checks for validity. Creates a query to store the
 * entry.
 * @param entry The entry to check and construct a query around.
 * @param schema The schema to check the entry against.
 * @param schema_count The size of the schema array.
 * @param query The pointer that will eventually point to the newly created
 * query.
 * @param table_name The target table's name.
 * @param primary_column_name The name of the primary column.
 * @param duplicate_column_name The name of the column that might be duplicated.
 * @returns 0 if the query is invalid, 1 if the query is valid, -1 on unexpected
 * failure.
 */
int construct_validate_query(json_t *entry, struct data_column *schema,
                             unsigned int schema_count, char **query,
                             char *table_name, const char *primary_column_name,
                             const char *duplicate_column_name) {
  const char *key = NULL;
  const json_t *value = NULL;
  char *value_string = NULL;
  size_t column_name_size = 0;
  char *column_name = NULL;

  size_t column_names_size = 1; // null-terminator
  char *column_names = NULL;
  size_t values_size = 1; // null-terminator
  char *values = NULL;

  // The status (return value) of the query validation and construction.
  int status = 1;

  // check for conformance
  json_object_foreach(entry, key, value) {
    // whether or not the value has passed validation.
    bool validated = false;
    value_string = json_to_string(value);

    column_name_size = strlen(key) + 1;

    // column type & suffix related variables. These need not be freed.
    enum column_type column_type = DATA;
    regex_t suffix_preg;
    regmatch_t suffix_matches[1 + 1];

    // check if the column is directly inside the schema or not.
    if (regcomp(&suffix_preg, "_comments|_latlong_accuracy|_altitude_accuracy$",
                REG_EXTENDED) != 0) {
      status = -1;
      goto construct_validate_query_end;
    }

    // regular column (one that's in the schema)
    if (regexec(&suffix_preg, key, 2, suffix_matches, 0) == REG_NOMATCH) {
      column_name_size = strlen(key) + 1;
      // special column (one that's not in the schema)
    } else {
      column_name_size = suffix_matches[1].rm_so;
      const char *suffix = key + suffix_matches[1].rm_so;

      if (strcmp(suffix, "_comments") == 0) {
        column_type = COMMENTS;
      } else if (strcmp(suffix, "_latlong_accuracy") == 0) {
        column_type = LATLONG_ACCURACY;
      } else if (strcmp(suffix, "_altitude_accuracy") == 0) {
        column_type = ALTITUDE_ACCURACY;
      } else {
        status = -1;
        goto construct_validate_query_end;
      }
    }

    column_name = malloc(column_name_size);

    // check for malloc failures
    if (!value_string || !column_name) {
      status = -1;
      goto construct_validate_query_end;
    }
    memcpy(column_name, key, column_name_size - 1);
    column_name[column_name_size - 1] = '\0';

    if (strcmp(column_name, "id") == 0) {
      // skip id column (postgres autoincrement should handle it)
      free(value_string);
      free(column_name);
      continue;

      // @TODO make sure postgres doesn't tweak out over incorrect next keys
      switch (regex_check("^[0-9]+$", 0, REG_EXTENDED, 0, value_string)) {
      case 0:
        status = 0;
        break;
      case 1:
        validated = true;
        break;
      default:
        status = -1;
        goto construct_validate_query_end;
      }
    } else if (strcmp(column_name, "primary_tag") == 0) {
      // it's hard to validate FOREIGN KEY so we'll let Postgres take care of
      // this.
      switch (regex_check("^[0-9]+$", 0, REG_EXTENDED, 0, value_string)) {
      case 0:
        status = 0;
        break;
      case 1:
        validated = true;
        break;
      default:
        status = -1;
        goto construct_validate_query_end;
      }
    } else
      for (int i = 0; i < schema_count; i++) {
        // find which entry in the schema matches

        if (str_cci_cmp(column_name, schema[i].name) == 0) {
          // validate against the column schema
          status = validate_column(value, schema[i], column_type);
          if (!status)
            goto construct_validate_query_end;
          break;
        }
      }

    // if the value does not have a related column,
    if (!validated) {
      status = 0;
      goto construct_validate_query_end;
    }
    // @todo optimize
    // add one because comma separation
    column_names_size += strlen(key) + 1;
    values_size +=
        strlen(value_string) + 1; // no need to use to_snake_case: it won't
                                  // change the length of the string.
    free(column_name);
    free(value_string);
  }
  column_name = NULL;
  value_string = NULL;

  // populate column_names & values

  column_names = malloc(column_names_size);
  values = malloc(values_size);

  // catch malloc failure
  if (!column_names || !values) {
    status = -1;
    goto construct_validate_query_end;
  }

  char *column_names_cur = column_names;
  char *values_cur = values;

  // build the query
  json_object_foreach(entry, key, value) {
    // skip ID column (rely on auto-increment)
    if (strcmp(key, "id") == 0)
      continue;

    value_string = json_to_string(value);

    // catch malloc failure
    if (!value_string) {
      status = -1;
      goto construct_validate_query_end;
    }
    // deal with snake case later
    memcpy(column_names_cur, key, strlen(key));
    column_names_cur += strlen(key);
    // add in the comma while moving the cursor forward
    *column_names_cur++ = ',';

    memcpy(values_cur, value_string, strlen(value_string));
    values_cur += strlen(value_string);
    // add in the comma while moving the cursor forward
    *values_cur++ = ',';

    free(value_string);
  }
  value_string = NULL;

  // remove trailing commas
  *--column_names_cur = '\0';
  *--values_cur = '\0';

  // abuse the fact that to_snake_case completely ignores commas
  to_lower_snake_case(column_names);

  size_t query_size =
      strlen("INSERT INTO  () VALUES() ON CONFLICT () DO UPDATE SET  = "
             "EXCLUDED. RETURNING ;") +
      strlen(table_name) + (column_names_size - 1) + (values_size - 1) +
      strlen(primary_column_name) + 3 * strlen(duplicate_column_name) + 1;
  *query = malloc(query_size);
  if (!*query) {
    status = -1;
    goto construct_validate_query_end;
  }
  snprintf(*query, query_size,
           "INSERT INTO %s (%s) VALUES(%s) ON CONFLICT (%s) DO UPDATE SET %s = "
           "EXCLUDED.%s RETURNING %s;",
           table_name, column_names, values, duplicate_column_name,
           duplicate_column_name, duplicate_column_name, primary_column_name);

construct_validate_query_end:
  free(column_name);
  free(value_string);
  free(column_names);
  free(values);
  return status;
}