#include "auth/test_creds.h"
#include "auth/test_login.h"
#include "auth/test_rng.h"
#include "auth/test_session.h"
#include "postgres/test_datatype_validation.h"
#include "postgres/test_search.h"
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

  RUN_TEST_SUITE(search_table_type_detection)
  RUN_TEST_SUITE(search_display_column)
  RUN_TEST_SUITE(search_q_parseability)
  RUN_TEST_SUITE(search_query_building)
  RUN_TEST_SUITE(search_response_serialization)

  puts("Testing complete.");
  return 0;
}