#include "auth/test_creds.h"
#include "auth/test_login.h"
#include "auth/test_rng.h"
#include "auth/test_session.h"
#include "postgres/test_datatype_validation.h"
#include <stdio.h>

int main() {
  // Auth
  test_generate_secure_random_string();
  test_session();
  test_creds();

  RUN_TEST_SUITE(login)
  RUN_TEST_SUITE(handle_login)

  // test_check_st_point();

  puts("Testing complete.");
  return 0;
}