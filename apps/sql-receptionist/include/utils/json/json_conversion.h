#include <jansson.h>
extern char *json_to_string(const json_t *value);

/**
 * Serializes JSON to a string value by writing to the given buffer. write_json_value is safe from buffer overflow. The output string is null terminated.
 * @param value The value to convert.
 * @param buffer The buffer to write the value to.
 * @param buffer_size The size of the buffer available to write to.
 * @returns The number of bytes written excluding the null terminator, or -1.
 */
extern size_t write_json_value(const json_t *value, char *buffer, size_t buffer_size);