#include "auth/rng.h"
#include <stdint.h>
#include <stdio.h>

// 32 -> 2^5
static const char randomStringAlphabet[32] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
    'm', 'n', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '2', '3', '4', '5', '6', '7', '8', '9'};

int generate_secure_random_string(char *dest) {
  FILE *f = fopen("/dev/urandom", "rb");
  if (!f)
    return 0;

  uint8_t bits_buffer[RANDOM_STRING_LENGTH];
  if (fread(bits_buffer, 1, RANDOM_STRING_LENGTH, f) != RANDOM_STRING_LENGTH) {
    fclose(f);
    return 0;
  }
  fclose(f);

  for (int i = 0; i < RANDOM_STRING_LENGTH; i++) {
    dest[i] = randomStringAlphabet[bits_buffer[i] & 0x1F];
  }

  return 1;
}