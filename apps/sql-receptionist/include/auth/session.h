#include <libpq-fe.h>

/**
 * Creates a RANDOM_STRING_LENGTH + 1 + RANDOM_STRING_LENGTH + 1 size token.
 * Also handles database INSERTION.
 * @param username A null terminated username for the related user.
 * @param token The token output buffer.
 * @param conn A connection to use in INSERTing the new session.
 * @returns Pass/fail. A success is defined as a successful token creation and
 * database INSERTion.
 */
extern int create_session(char *username, char *token, PGconn *conn);