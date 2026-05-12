#include <libpq-fe.h>
#include <stddef.h>
/**
 * Writes an HTTP response based on the given body to a authentication request.
 * @param response
 * @param response_len
 * @param body
 */
extern void handle_login(char **response, size_t *response_len,
                         const char *body);

/**
 * Attempts to login a user based on their username and password. If the login
 * is successful, the respective database queries would have already run to
 * completion.
 * @param token Token output pointer. This pointer should have the size of a
 * null terminated token.
 * @param username
 * @param password
 * @param conn The connection to use when INSERTing a new session.
 * @returns Whether or not the login was successful (i.e. whether or not the
 * token is valid)
 */
extern int login(char *token, const char *username, const char *password,
                 PGconn *conn);