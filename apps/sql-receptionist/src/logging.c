#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static FILE *logger = NULL;
static FILE *debug_logger = NULL;

// @TODO log queue to avoid interleaving messages

static pthread_once_t initialize_loggers_frequency = PTHREAD_ONCE_INIT;

static void initialize_loggers() {
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

void log_critical(const char *message) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  fputs(message, stderr);
  fputs(message, logger);
  fputs(message, debug_logger);
}
void log_error(const char *message) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  fputs(message, stderr);
  fputs(message, logger);
  fputs(message, debug_logger);
}
void log_warn(const char *message) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  fputs(message, stderr);
  fputs(message, logger);
  fputs(message, debug_logger);
}
void log_info(const char *message) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  fputs(message, stderr);
  fputs(message, logger);
  fputs(message, debug_logger);
}
void log_debug(const char *message) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);
  fputs(message, debug_logger);
}

void log_critical_printf(const char *format, ...) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  va_list arg;
  va_start(arg, format);
  va_list arg1, arg2;
  va_copy(arg1, arg);
  va_copy(arg2, arg);

  vfprintf(stderr, format, arg);
  vfprintf(logger, format, arg1);
  vfprintf(debug_logger, format, arg2);

  va_end(arg);
  va_end(arg1);
  va_end(arg2);
}
void log_error_printf(const char *format, ...) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  va_list arg;
  va_start(arg, format);
  va_list arg1, arg2;
  va_copy(arg1, arg);
  va_copy(arg2, arg);

  vfprintf(stderr, format, arg);
  vfprintf(logger, format, arg1);
  vfprintf(debug_logger, format, arg2);

  va_end(arg);
  va_end(arg1);
  va_end(arg2);
}
void log_warn_printf(const char *format, ...) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  va_list arg;
  va_start(arg, format);
  va_list arg1, arg2;
  va_copy(arg1, arg);
  va_copy(arg2, arg);

  vfprintf(stderr, format, arg);
  vfprintf(logger, format, arg1);
  vfprintf(debug_logger, format, arg2);

  va_end(arg);
  va_end(arg1);
  va_end(arg2);
}
void log_info_printf(const char *format, ...) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  va_list arg;
  va_start(arg, format);
  va_list arg1, arg2;
  va_copy(arg1, arg);
  va_copy(arg2, arg);

  vfprintf(stdout, format, arg);
  vfprintf(logger, format, arg1);
  vfprintf(debug_logger, format, arg2);

  va_end(arg);
  va_end(arg1);
  va_end(arg2);
}
void log_debug_printf(const char *format, ...) {
  pthread_once(&initialize_loggers_frequency, initialize_loggers);

  va_list arg;
  va_start(arg, format);

  vfprintf(debug_logger, format, arg);

  va_end(arg);
}