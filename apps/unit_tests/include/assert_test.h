#include <libpq-fe.h>

/**
 * Checks a value for truthiness and logs an error if it is not truthy.
 * @param value
 * @param message
 * @returns pass/fail
 */
extern int assert_true(int value, const char *message);
/**
 * Checks a value for falsiness and logs and error if it is not falsy.
 * @param value
 * @param message
 * @returns pass/fail
 */
extern int assert_false(int value, const char *message);

/**
 * Connects to a database and treats a failed connection like a failure.
 * Assumes errno is initially 0.
 * @param conn The output for the connection pointer. This pointer is NULL or
 * dangling if the connection fails.
 * @param database_name
 * @returns pass/fail
 */
extern int assert_database_connection(PGconn **conn, const char *database_name);

/**
 * Attempts to read from a file and treats a failed read like a failure
 * @param buffer The output buffer.
 * @param n The output buffer size.
 * @param filepath The path to the server-side file.
 * @param mode The mode to read the file.
 * @returns pass/fail
 */
extern int assert_file_readable(char *buffer, size_t n, const char *filepath,
                                const char *mode);

extern inline int assert_response_status(char *response, int status,
                                         const char *message) {
  if (!response)
    return 0;
  // HTTP/1.1 xxx
  if (response[11] != status % 10) {
    puts(message);
    return 0;
  }
  status /= 10;
  if (response[10] != status % 10) {
    puts(message);
    return 0;
  }
  status /= 10;
  if (response[9] != status % 10) {
    puts(message);
    return 0;
  }
  return 1;
}

extern int assert_response_body(char *response, const char *body,
                                const char *message);