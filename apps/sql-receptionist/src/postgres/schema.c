#ifndef HEADER_CONFIG
#define HEADER_CONFIG
#include "config.h"
#endif
#include "utils/regex_item.h"
#include <jansson.h>
#include <regex.h>
#include <string.h>

/**
 * Checks whether or not a given value is in the format xxxx-xx-xx
 * @param json The value to validate.
 * @return 0 if it is not a valid date, 1 if it's date-like
 */
int check_datelike(const json_t *json) {
  if (json_is_string(json)) {
    const char *text = json_string_value(json);
    regex_t check_regex;
    regcomp(&check_regex, "'[0-9]{1,4}-[0-9]{2}-[0-9]{2}'", REG_EXTENDED);
    // Alternative pattern (does not account for february 29):
    // xxxx-[1<=num<=12]-[valid date within that month]:
    // ([0-9]{1,4})-?((02)-?([0]|[12][0-9])|(01|03|05|07|08|10|12)-?(0[1-9]|[12][0-9]|3[01])|(04|06|09|11)-?(0[1-9]|[12][0-9]|30))

    regmatch_t check_matches[4];

    if (regexec(&check_regex, text, 4, check_matches, 0) == REG_NOMATCH) {
      regfree(&check_regex);
      return 0;
    }

    if (check_matches[0].rm_eo == -1 || check_matches[0].rm_so == -1) {
      regfree(&check_regex);
      return 0;
    }

    // make sure it's just a datelike string and nothing else is after that.
    int match_size = check_matches[0].rm_eo - check_matches[0].rm_so;
    regfree(&check_regex);
    if (match_size == strlen(text)) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}
/**
 * Checks whether or not a given value is in the format of xx:xx:xxZ or TxxxxxxZ
 * or xx:xx:xx.x... or Txxxxxx...
 */
int check_timelike(const json_t *json) {
  if (json_is_string(json)) {
    const char *text = json_string_value(json);
    regex_t check_regex;
    regcomp(&check_regex,
            "'([0-9]{2}:[0-9]{2}:[0-9]{2}|[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{1,6}"
            "|T[0-9]{6}|T[0-9]{6}.[0-9]{1,6})'",
            REG_EXTENDED);

    regmatch_t check_matches[2];

    if (regexec(&check_regex, text, 2, check_matches, 0) == REG_NOMATCH) {
      regfree(&check_regex);
      return 0;
    }

    if (check_matches[0].rm_eo == -1 || check_matches[0].rm_so == -1) {
      regfree(&check_regex);
      return 0;
    }

    // make sure it's just a timelike string and nothing else is after that.
    int match_size = check_matches[0].rm_eo - check_matches[0].rm_so;
    regfree(&check_regex);
    if (match_size == strlen(text)) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}
int check_timestamplike(const json_t *json) {
  if (json_is_string(json)) {
    const char *text = json_string_value(json);
    regex_t check_regex;
    regcomp(&check_regex,
            "'([0-9]{1,4}-[0-9]{2}-[0-9]{2})T([0-9]{2}:[0-9]{2}:[0-9]{2}|[0-9]{"
            "2}:[0-9]{2}:[0-9]{2}.[0-9]{1,6}|T[0-9]{6}|T[0-9]{6}.[0-9]{1,6})'",
            REG_EXTENDED);

    regmatch_t check_matches[2];

    if (regexec(&check_regex, text, 2, check_matches, 0) == REG_NOMATCH) {
      regfree(&check_regex);
      return 0;
    }

    if (check_matches[0].rm_eo == -1 || check_matches[0].rm_so == -1) {
      regfree(&check_regex);
      return 0;
    }

    // make sure it's just a timelike string and nothing else is after that.
    int match_size = check_matches[0].rm_eo - check_matches[0].rm_so;
    regfree(&check_regex);
    if (match_size == strlen(text)) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}
int check_st_point(const json_t *json) {
  return regex_check(
      "^POINT(?:\\s+(?:Z|M|ZM))?\\s*\\(\\s*(?:EMPTY|[-+]?\\d*\\.?\\d+(?"
      ":[eE][-+]?\\d+)?\\s+[-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?(?:\\s+[-"
      "+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?){0,2})\\s*\\)$",
      2, REG_EXTENDED, 0, json_string_value(json));
}

int validate_column(const json_t *item, struct data_column column_schema,
                    enum column_type column_type) {
  switch (column_type) {
  case DATA:
    const char *datatype = column_schema.datatype;
    int output = -1;
    if (strcmp(datatype, "int") == 0 || strcmp(datatype, "integer") == 0) {
      output = json_is_integer(item);
    } else if (strcmp(datatype, "float") == 0 ||
               strcmp(datatype, "number") == 0) {
      output = json_is_real(item);
    } else if (strcmp(datatype, "string") == 0 ||
               strcmp(datatype, "str") == 0 || strcmp(datatype, "text") == 0) {
      output = json_is_string(item);
    } else if (strcmp(datatype, "bool") == 0 ||
               strcmp(datatype, "boolean") == 0) {
      output = json_is_boolean(item);
    } else if (strcmp(datatype, "date") == 0) {
      output = check_datelike(item);
    } else if (strcmp(datatype, "time") == 0) {
      output = check_timelike(item);
    } else if (strcmp(datatype, "timestamp") == 0) {
      output = check_timestamplike(item);
    } else if (strcmp(datatype, "enum") == 0) {
      // @todo
      output = json_is_string(item);
    } else if (strcmp(datatype, "geodetic point") == 0) {
      output = check_st_point(item);
    } else {
      if (getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES") &&
          strcmp(getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES"), "TRUE") == 0)
        printf("Column %s does not have a valid datatype (%s).\n",
               column_schema.name, datatype);
      return 0;
    }

    if (getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES") &&
        strcmp(getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES"), "TRUE") == 0) {
      switch (output) {
      case 0:
        printf("Non-conformant data against column %s.\n", column_schema.name);
        break;
      case -1:
        printf("Something went wrong while trying to validate the datatype "
               "against column %s.\n",
               column_schema.name);
        break;
      }
    }
    return output;
  case COMMENTS:
    if (column_schema.comments == false) {
      if (getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES") &&
          strcmp(getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES"), "TRUE") == 0)
        printf("Column %s does not have commenting enabled.\n",
               column_schema.name);
      return 0;
    }

    // comments should be strings.
    if (json_is_string(item) == 0) {
      if (getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES") &&
          strcmp(getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES"), "TRUE") == 0)
        printf("The comment for column %s was empty.\n", column_schema.name);
      return 0;
    }
    return 1;
  case LATLONG_ACCURACY:
  case ALTITUDE_ACCURACY:
    // the related column should be a geodetic point.
    if (strcmp(column_schema.datatype, "geodetic point") != 0) {
      if (getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES") &&
          strcmp(getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES"), "TRUE") == 0)
        printf("Column %s is not a geodetic point and therefore cannot have a "
               "related geodetic accuracy.\n",
               column_schema.name);
      return 0;
    }

    // accuracy should be a double precision.
    if (!json_is_real(item)) {
      if (getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES") &&
          strcmp(getenv("SQL_RECEPTIONIST_LOG_SCHEMA_FAILURES"), "TRUE") == 0)
        printf(
            "Non-conformat data. Expected a double precision for column %s.\n",
            column_schema.name);
      return 0;
    }
    return 1;
  }
}