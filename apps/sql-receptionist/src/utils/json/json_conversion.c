#include <errno.h>
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
  case JSON_NULL:
    output_size = 5;
    output = malloc(5);
    output[0] = 'N';
    output[1] = 'U';
    output[2] = 'L';
    output[3] = 'L';
    output[4] = '\0';
    return output;
  default:
    return NULL;
  }
}

size_t write_json_value(const json_t *value, char *buffer, size_t buffer_size) {
  size_t n = -1;
  switch (json_typeof(value)) {
  case JSON_STRING:;
    const char *str = json_string_value(value);
    n = strlen(str);
    if (n >= buffer_size) {
      n = -1;
      errno = ENOMEM;
      break;
    }

    memcpy(buffer, str, n + 1);
    break;
  case JSON_INTEGER:
    n = snprintf(buffer, buffer_size, "%lld",
                 (long long)json_integer_value(value));
    if (n >= buffer_size) {
      n = -1;
      errno = ENOMEM;
    }
    break;
  case JSON_REAL:
    n = snprintf(buffer, buffer_size, "%.17g", json_real_value(value));
    if (n >= buffer_size) {
      n = -1;
      errno = ENOMEM;
    }
    break;
  case JSON_TRUE:
    if (buffer_size < 5) {
      errno = ENOMEM;
      break;
    }

    buffer[0] = 't';
    buffer[1] = 'r';
    buffer[2] = 'u';
    buffer[3] = 'e';
    buffer[4] = '\0';
    n = 5;
    break;
  case JSON_FALSE:
    if (buffer_size < 6) {
      errno = ENOMEM;
      break;
    }

    buffer[0] = 'f';
    buffer[1] = 'a';
    buffer[2] = 'l';
    buffer[3] = 's';
    buffer[4] = 'e';
    buffer[5] = '\0';
    n = 5;
    break;
  case JSON_NULL:
    if (buffer_size < 5) {
      errno = ENOMEM;
      break;
    }

    buffer[0] = 'N';
    buffer[1] = 'U';
    buffer[2] = 'L';
    buffer[3] = 'L';
    buffer[4] = '\0';
    n = 4;
    break;
  default:
    errno = EINVAL;
  }

  return n;
}