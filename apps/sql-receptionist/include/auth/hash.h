#include <argon2.h>
#include <stddef.h>

#define ARGON2_PHC_SIZE 101

/**
 * Creates an Argon2id PHC string based on the given input.
 * @param input The input to hash.
 * @param input_len The length of the input to has in bytes.
 * @param phc_out The output buffer for the PHC string with a minimum size of
 * ARGON2_PHC_SIZE.
 * @returns The resulting Argon2 exit code.
 */
extern enum Argon2_ErrorCodes argon2id_phc(void *input, size_t input_len,
                                           char *phc_out);

/**
 * Hashes an input with SHA-256 and outputs a hex string.
 * Is guarenteed to succeed.
 * @param input The input to hash.
 * @param input_len The length of the input to hash in bytes.
 * @param hash_hex The output buffer with a minimize size of 32 bytes.
 */
extern void sha_256_hex(void *input, size_t input_len, char *hash_hex);