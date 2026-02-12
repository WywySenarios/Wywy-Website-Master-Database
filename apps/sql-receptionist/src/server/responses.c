/**
 * @brief helper library for building HTTP 1.1 responses.
 * This helper library can build responses for the following status codes: 200,
 * 204, 400, 403, 404, 500.
 */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/**
 * Returns the name associated with the given status code.
 * @return the name associated with the given status code.
 */
const char *get_status_code_name(int status_code) {
  switch (status_code) {
  case 200:
    return "OK";
  case 204:
    return "No Content";
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 500:
    return "Internal Server Error";
  default:
    return "";
  }
}

/**
 * Build a response, and set its body to the given string. Fails if the config
 * is missing or NULL.
 * @param status_code The status code of the response.
 * @param response response text output pointer.
 * @param response_len response text length output pointer.
 * @param body response body string.
 */
void build_response(int status_code, char **response, size_t *response_len,
                    char *body) {
  const char *status_code_name = get_status_code_name(status_code);

  if (getenv("SQL_RECEPTIONIST_LOG_RESPONSES") &&
      strcmp(getenv("SQL_RECEPTIONIST_LOG_RESPONSES"), "TRUE") == 0)
    printf("Constructing %d %s response: %s\n\n", status_code, status_code_name,
           body);

  const char *connection = "Close";
  if (status_code == 204) {
    connection = "Keep-Alive";
  }

  *response_len = strlen("HTTP/1.1 xxx \r\n"
                         "Content-Type: text/plain\r\n"
                         "Access-Control-Allow-Origin: \r\n"
                         "Access-Control-Allow-Headers: Content-Type\r\n"
                         "Access-Control-Allow-Credentials: true\r\n"
                         "Connection: \r\n"
                         "\r\n") +
                  strlen(status_code_name) + strlen(getenv("MAIN_URL")) +
                  strlen(connection) + strlen(body);
  *response = malloc(*response_len + 1);
  if (!*response) {
    perror("Malloc failure on *response.");
    return;
  }
  snprintf(*response, *response_len + 1,
           "HTTP/1.1 %d %s\r\n"
           "Content-Type: text/plain\r\n"
           "Access-Control-Allow-Origin: %s\r\n"
           "Access-Control-Allow-Headers: Content-Type\r\n"
           "Access-Control-Allow-Credentials: true\r\n"
           "Connection: %s\r\n"
           "\r\n%s",
           status_code, status_code_name, getenv("MAIN_URL"), connection, body);
}

/**
 * Build a response when supplied with snprintf style arguments (pattern &
 * variable number of arguments). Fails if the config is missing or NULL.
 * @param status_code The status code of the response.
 * @param response response text output pointer.
 * @param response_len response text length output pointer.
 * @param text_size expected body size. Does not include the null terminator
 * (i.e. will malloc text_size + 1)
 * @param pattern body string pattern.
 * @param ... body string content (args for vsnprintf).
 */
void build_response_printf(int status_code, char **response,
                           size_t *response_len, size_t text_size,
                           const char *pattern, ...) {
  va_list arg;
  char *body = malloc(text_size + 1);
  va_start(arg, pattern);
  vsnprintf(body, text_size + 1, pattern, arg);
  va_end(arg);

  build_response(status_code, response, response_len, body);

  free(body);
}

/**
 * Build a response with the status code name as the body. Fails if the config
 * is missing or NULL.
 * @param status_code The status code of the response.
 * @param response response text output pointer.
 * @param response_len response text length output pointer.
 */
void build_response_default(int status_code, char **response,
                            size_t *response_len) {
  switch (status_code) {
  case 200:
    build_response(200, response, response_len, "OK");
    break;
  case 204:
    build_response(204, response, response_len, "No Content");
    break;
  case 400:
    build_response(400, response, response_len, "Bad Request");
    break;
  case 403:
    build_response(403, response, response_len, "Forbidden");
    break;
  case 404:
    build_response(404, response, response_len, "Not Found");
    break;
  case 500:
    build_response(500, response, response_len, "Internal Server Error");
    break;
  }
}