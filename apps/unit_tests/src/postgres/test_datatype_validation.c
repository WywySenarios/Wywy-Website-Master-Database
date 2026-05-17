#include "assert_test.h"
#include "postgres/schema.h"
#include "test_case.h"
#include <jansson.h>

void test_check_st_point() {
  json_t *val = json_string("POINT (0 0)");
  assert_true(
      check_st_point(val),
      "F: A valid value did not pass datatype validation: POINT (0 0)\n");
  json_decref(val);
}

TEST_SUITE(pointer_datatype) {
  int pass = 1;
  struct data_column col = {"test", "pointer", false, "", NULL};
  json_t *int_val = json_integer(42);
  json_t *str_val = json_string("hello");
  json_t *real_val = json_real(3.14);
  json_t *bool_val = json_true();

  pass &= assert_true(validate_column(int_val, col, DATA),
                      "F: pointer should accept integers\n");
  pass &= assert_false(validate_column(str_val, col, DATA),
                       "F: pointer should reject strings\n");
  pass &= assert_false(validate_column(real_val, col, DATA),
                       "F: pointer should reject floats\n");
  pass &= assert_false(validate_column(bool_val, col, DATA),
                       "F: pointer should reject booleans\n");

  json_decref(int_val);
  json_decref(str_val);
  json_decref(real_val);
  json_decref(bool_val);
  return pass;
}

TEST_SUITE(polypointer_datatype) {
  int pass = 1;
  struct data_column col = {"test", "polypointer", false, "", NULL};
  json_t *int_val = json_integer(42);
  json_t *str_val = json_string("hello");
  json_t *real_val = json_real(3.14);

  pass &= assert_true(validate_column(int_val, col, DATA),
                      "F: polypointer should accept integers\n");
  pass &= assert_false(validate_column(str_val, col, DATA),
                       "F: polypointer should reject strings\n");
  pass &= assert_false(validate_column(real_val, col, DATA),
                       "F: polypointer should reject floats\n");

  json_decref(int_val);
  json_decref(str_val);
  json_decref(real_val);
  return pass;
}

TEST_SUITE(polymorphic_pointer_datatype) {
  int pass = 1;
  struct data_column col = {"test", "polymorphic pointer", false, "", NULL};
  json_t *int_val = json_integer(42);
  json_t *str_val = json_string("hello");

  pass &= assert_true(validate_column(int_val, col, DATA),
                      "F: polymorphic pointer should accept integers\n");
  pass &= assert_false(validate_column(str_val, col, DATA),
                       "F: polymorphic pointer should reject strings\n");

  json_decref(int_val);
  json_decref(str_val);
  return pass;
}

TEST_SUITE(pointer_type_subcolumn) {
  int pass = 1;
  struct data_column col = {"test", "polypointer", false, "", NULL};
  json_t *str_val = json_string("users");
  json_t *null_val = json_null();
  json_t *int_val = json_integer(42);
  json_t *bool_val = json_true();

  pass &= assert_true(validate_column(str_val, col, POINTER_TYPE),
                      "F: pointer _type should accept strings\n");
  pass &= assert_true(validate_column(null_val, col, POINTER_TYPE),
                      "F: pointer _type should accept null (optional)\n");
  pass &= assert_false(validate_column(int_val, col, POINTER_TYPE),
                       "F: pointer _type should reject integers\n");
  pass &= assert_false(validate_column(bool_val, col, POINTER_TYPE),
                       "F: pointer _type should reject booleans\n");

  json_decref(str_val);
  json_decref(null_val);
  json_decref(int_val);
  json_decref(bool_val);
  return pass;
}