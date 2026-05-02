#define RANDOM_STRING_LENGTH 24

/**
 * Generate a cryptographically secure random string of
 * length RANDOM_STRING_LENGTH that is not null terminated.
 * @param dest The destination to write the random string to.
 */
extern int generateSecureRandomString(char *dest);