#include <libpq-fe.h>

#define MAX_PASSWORD_LENGTH 256

/**
 * Checks the given credentials. The password hash is checked in constant time.
 * @param username The username of the user to attempt to authenticate
 * @param password A null-terminated password assumed to have a max size of
 * MAX_PASSWORD_LENGTH
 * + 1
 * @param conn
 */
extern int check_creds(char *username, char *password, PGconn *conn);