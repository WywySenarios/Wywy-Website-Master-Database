#include <jansson.h>
#include <string.h>
char *json_to_string(const json_t *value) {
  size_t output_size;
  char *output;
  if (!value) {
    return NULL;
  }

  switch (json_typeof(value)) {
  case JSON_STRING:
    output_size = strlen(json_string_value(value)) + 1;
    output = malloc(output_size);
    snprintf(output, output_size, "%s", json_string_value(value));
    return output;
  case JSON_INTEGER:
    output_size = 32 + 1;
    output = malloc(output_size);
    snprintf(output, output_size, "%lld", (long long)json_integer_value(value));
    return output;
  case JSON_REAL:
    output_size = 64 + 1;
    output = malloc(output_size);
    snprintf(output, output_size, "%.17g", json_real_value(value));
    return output;
  case JSON_TRUE:
    output_size = 4 + 1;
    output = malloc(output_size);
    snprintf(output, output_size, "true");
    return output;
  case JSON_FALSE:
    output_size = 5 + 1;
    output = malloc(6);
    snprintf(output, output_size, "false");
    return output;
  default:
    return NULL;
  }
}