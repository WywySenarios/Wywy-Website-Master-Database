#include "assert_test.h"
#include "auth/creds.h"
#include "logging.h"
#include "postgres.h"
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>

int test_creds() {
  PGconn *conn = NULL;
  if (!assert_database_connection(&conn, "info"))
    return 0;
  char admin_password[257];
  if (!assert_file_readable(admin_password, 257, "/run/secrets/admin", "r"))
    return 0;

  int pass = 1;

  pass &= assert_false(check_creds("notadmin", admin_password, conn),
                       "F: An invalid credential (wrogn username)");
  pass &=
      assert_true(check_creds("admin", admin_password, conn),
                  "F: A valid credential did not pass credential validation.");

  // assume the admin password is not empty. That would be really stupid.
  admin_password[0] = '\0';
  pass &= assert_false(check_creds("admin", admin_password, conn),
                       "F: An invalid credential (wrong password) passed "
                       "credential validation.");

  return pass;
}