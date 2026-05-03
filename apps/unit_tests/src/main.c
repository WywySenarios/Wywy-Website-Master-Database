#include "auth/test_rng.h"
#include "auth/test_session.h"
#include "postgres/test_datatype_validation.h"

int main() {
  // Auth
  test_generate_secure_random_string();
  test_session_creation();

  // test_check_st_point();

  return 0;
}