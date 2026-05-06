#include "auth/rng.h"
#include <libpq-fe.h>

#define TOKEN_LENGTH RANDOM_STRING_LENGTH + 1 + RANDOM_STRING_LENGTH

/**
 * Creates a TOKEN_LENGTH + 1 size token.
 * Also handles database INSERTION.
 * @param username A null terminated username for the related user.
 * @param token The token output buffer.
 * @param conn A connection to use in INSERTing the new session.
 * @returns Pass/fail. A success is defined as a successful token creation and
 * database INSERTion.
 */
extern int create_session(char *username, char *token, PGconn *conn);

/**
 * Validates a token and accesses the related username if the token is valid.
 * @param username The username output buffer.
 * @param token The token to validate.
 * @param conn A database connection to use when validating the token.
 * @returns Whether or not the token is valid.
 */
extern int validate_token(char *username, char *token, PGconn *conn);