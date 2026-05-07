#include <stdlib.h>
/**
 * Write the respective HTTP headers to the given buffer. Prevents buffer
 * overflow with snprintf.
 * @param status_code The status code of the HTTP response.
 * @param buffer The buffer to write to.
 * @param buffer_size The available size of the buffer.
 * @returns The size that would have been written if the buffer was infinitely
 * large.
 */
extern size_t write_header(const int status_code, char *buffer,
                           const size_t buffer_size);
extern void build_response(int status_code, char **response,
                           size_t *response_len, const char *headers,
                           const char *body);
extern void build_response_printf(int status_code, char **response,
                                  size_t *response_len, const char *headers,
                                  size_t text_size, const char *pattern, ...);
extern void build_response_default(int status_code, char **response,
                                   size_t *response_len);