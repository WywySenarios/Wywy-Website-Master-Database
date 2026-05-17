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

  test_check_st_point();
  RUN_TEST_SUITE(pointer_datatype)
  RUN_TEST_SUITE(polypointer_datatype)
  RUN_TEST_SUITE(polymorphic_pointer_datatype)
  RUN_TEST_SUITE(pointer_type_subcolumn)

  puts("Testing complete.");
  return 0;
}