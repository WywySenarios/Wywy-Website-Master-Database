#include "auth/hash.h"
#include <argon2.h>
#include <openssl/evp.h>

#define ARGON2_T_COST 3u      /* iterations                   */
#define ARGON2_M_COST 65536u  /* memory in KiB (64 MiB)       */
#define ARGON2_PARALLELISM 4u /* threads / lanes              */
#define ARGON2_HASH_LEN 32u   /* raw digest bytes (256-bit)   */
#define ARGON2_SALT_LEN 16u   /* salt bytes                   */

enum Argon2_ErrorCodes argon2id_phc(void *input, size_t input_len,
                                    char *phc_out) {
  if (!input || !phc_out)
    return ARGON2_INCORRECT_PARAMETER;
  // generate salt
  uint8_t salt[ARGON2_SALT_LEN];
  FILE *urandom = fopen("/dev/urandom", "rb");
  if (!urandom)
    return ARGON2_SALT_PTR_MISMATCH;
  if (fread(salt, 1, sizeof(salt), urandom) != sizeof(salt))
    return ARGON2_SALT_PTR_MISMATCH;
  fclose(urandom);

  return argon2id_hash_encoded(ARGON2_T_COST, ARGON2_M_COST, ARGON2_PARALLELISM,
                               input, input_len, salt, ARGON2_SALT_LEN,
                               ARGON2_HASH_LEN, phc_out, ARGON2_PHC_SIZE);
}

void sha_256_hex(void *input, size_t input_len, char *hash_hex) {
  unsigned char secretHash[EVP_MAX_MD_SIZE];
  unsigned int secretHashLength;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
  EVP_DigestUpdate(ctx, input, input_len);
  EVP_DigestFinal_ex(ctx, secretHash, &secretHashLength);
  EVP_MD_CTX_free(ctx);

  for (unsigned int i = 0; i < secretHashLength; i++)
    sprintf(hash_hex + i * 2, "%02x", secretHash[i]);
  hash_hex[64] = '\0';
}