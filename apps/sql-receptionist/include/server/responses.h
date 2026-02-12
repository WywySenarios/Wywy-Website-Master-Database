#include <stdlib.h>
extern void build_response(int status_code, char **response,
                           size_t *response_len, char *body);
extern void build_response_printf(int status_code, char **response,
                                  size_t *response_len, size_t text_size,
                                  const char *pattern, ...);
extern void build_response_default(int status_code, char **response,
                                   size_t *response_len);