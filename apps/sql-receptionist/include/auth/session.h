#include "auth/rng.h"
#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_LENGTH RANDOM_STRING_LENGTH + 1 + RANDOM_STRING_LENGTH
// Set-Cookie session=; Max-Age: 34560000; HttpOnly; Path=/\r\n
#define TOKEN_COOKIE_HEADER_MAX_LENGTH                                         \
  (sizeof("Set-Cookie: session=") + TOKEN_LENGTH +                             \
   sizeof("; Max-Age: 34560000; HttpOnly; Path=/\r\n") - 2)
#define COOKIE_HEADER_TOKEN_OFFSET (sizeof("Set-Cookie session=") - 1)

/**
 * Writes the cookie header except the token, which is expected to be inputted
 * manually.
 * @param cookie_header The cookie header output buffer.
 * @returns The location to write the token.
 */
static inline char *write_cookie_header(char *cookie_header) {
  memcpy(cookie_header,
         "Set-Cookie: session=", sizeof("Set-Cookie: session=") - 1);
  char *out = cookie_header + sizeof("Set-Cookie: session=") - 1;
  char *pos = out + TOKEN_LENGTH;
  memcpy(pos, "; Max-Age: ", sizeof("; Max-Age: ") - 1);
  pos += sizeof("; Max-Age: ") - 1;
  const char *max_age = getenv("AUTH_COOKIE_MAX_AGE");
  size_t max_age_len = strlen(max_age);
  memcpy(pos, max_age, max_age_len);
  pos += max_age_len;
  memcpy(pos, "; HttpOnly; Path=/\r\n", sizeof("; HttpOnly; Path=/\r\n") - 1);
  pos += sizeof("; HttpOnly; Path=/\r\n") - 1;
  *pos = '\0';
  return out;
}

/**
 * Creates a TOKEN_LENGTH + 1 size token.
 * Also handles database INSERTION.
 * @param username A null terminated username for the related user.
 * @param token The token output buffer.
 * @param conn A connection to use in INSERTing the new session.
 * @returns Pass/fail. A success is defined as a successful token creation and
 * database INSERTion.
 */
extern int create_session(const char *username, char *token, PGconn *conn);

/**
 * Validates a token and accesses the related username if the token is valid.
 * @param username The username output buffer.
 * @param token The token to validate.
 * @param conn A database connection to use when validating the token.
 * @returns Whether or not the token is valid.
 */
extern int validate_token(char *username, const char *token, PGconn *conn);