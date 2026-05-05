#include "auth/test_creds.h"
#include "auth/test_rng.h"
#include "auth/test_session.h"
#include "postgres/test_datatype_validation.h"
#include <stdio.h>

int main() {
  // Auth
  test_generate_secure_random_string();
  test_session();
  test_creds();

  // test_check_st_point();

  puts("Testing complete.");
  return 0;
}