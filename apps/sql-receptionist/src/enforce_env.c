#include "enforce_env.h"
#include "logging.h"
#include <stdlib.h>

#define check_env_var(name)                                                    \
  do {                                                                         \
    if (!getenv(name)) {                                                       \
      log_critical_printf("Could not find environment variable %s.\n", name);  \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

void enforce_environment_variables() {
  check_env_var("DATABASE_HOST");
  check_env_var("DATABASE_PORT");
  check_env_var("DATABASE_USERNAME");
  check_env_var("DATABASE_PASSWORD");
  check_env_var("MAIN_URL");
  check_env_var("CACHE_URL");
  check_env_var("AUTH_COOKIE_MAX_AGE");
  check_env_var("SQL_RECEPTIONIST_HOST");
  check_env_var("USER_ID");
}