
#include "assert_test.h"
#include "auth/rng.h"
#include "stdio.h"

int test_generate_secure_random_string() {
  char string[RANDOM_STRING_LENGTH];

  generateSecureRandomString(string);

  // assume that the function has succeeded if there are no null characters.
  for (int i = 0; i < RANDOM_STRING_LENGTH; i++) {
    if (string[i] == '\0') {
      printf("The random secure string is not long enough. Expected a string "
             "with length %d. Received a string with length %d.",
             RANDOM_STRING_LENGTH, i + 1);
      return 0;
    }
  }

  return 1;
}