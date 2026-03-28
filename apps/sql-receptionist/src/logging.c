#include <stdio.h>
#include <stdlib.h>

FILE *logger = NULL;
FILE *debug_logger = NULL;

void initialize_logger() {
  logger =
      fopen("/var/log/Wywy-Website/master-database/sql-receptionist.log", "a");

  if (!logger) {
    perror("Logger initialization");
    exit(EXIT_FAILURE);
  }

  debug_logger = fopen(
      "/var/log/Wywy-Website/master-database/sql-receptionist.debug.log", "a");

  if (!debug_logger) {
    perror("Debug logger initialization");
    exit(EXIT_FAILURE);
  }
}