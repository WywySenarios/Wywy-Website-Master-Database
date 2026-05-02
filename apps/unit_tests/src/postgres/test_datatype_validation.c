#include "assert_test.h"
#include "postgres/schema.h"
#include <jansson.h>

void test_check_st_point() {
  assert_true(check_st_point(json_string("POINT (0 0)")),
              "A valid value did not pass datatype validation: POINT (0 0)");
}