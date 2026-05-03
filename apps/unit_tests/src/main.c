#include "auth/test_rng.h"
#include "postgres/test_datatype_validation.h"

int main() {
  // Auth
  test_generate_secure_random_string();

  // test_check_st_point();

  return 0;
}