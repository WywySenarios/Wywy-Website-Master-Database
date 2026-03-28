#include <stdio.h>

#ifndef LOGGER_INITIALIZED
#define LOGGER_INITIALIZED

void initialize_logger();

#endif

extern FILE *logger;
extern FILE *debug_logger;

#define log_critical(text)                                                     \
  do {                                                                         \
    fputs(text, stderr);                                                       \
    fputs(text, logger);                                                       \
    fputs(text, debug_logger);                                                 \
  } while (0)
#define log_error(text)                                                        \
  do {                                                                         \
    fputs(text, stderr);                                                       \
    fputs(text, logger);                                                       \
    fputs(text, debug_logger);                                                 \
  } while (0)
#define log_warn(text)                                                         \
  do {                                                                         \
    fputs(text, stderr);                                                       \
    fputs(text, logger);                                                       \
    fputs(text, debug_logger);                                                 \
  } while (0)
#define log_info(text)                                                         \
  do {                                                                         \
    puts(text);                                                                \
    fputs(text, logger);                                                       \
    fputs(text, debug_logger);                                                 \
  } while (0)
#define log_debug(text)                                                        \
  do {                                                                         \
    fputs(text, debug_logger);                                                 \
  } while (0)

#define log_critical_printf(format, ...)                                       \
  do {                                                                         \
    fprintf(stderr, format, __VA_ARGS__);                                      \
    fprintf(logger, format, __VA_ARGS__);                                      \
    fprintf(debug_logger, format, __VA_ARGS__);                                \
  } while (0)
#define log_error_printf(format, ...)                                          \
  do {                                                                         \
    fprintf(stderr, format, __VA_ARGS__);                                      \
    fprintf(logger, format, __VA_ARGS__);                                      \
    fprintf(debug_logger, format, __VA_ARGS__);                                \
  } while (0)
#define log_warn_printf(format, ...)                                           \
  do {                                                                         \
    fprintf(stderr, format, __VA_ARGS__);                                      \
    fprintf(logger, format, __VA_ARGS__);                                      \
    fprintf(debug_logger, format, __VA_ARGS__);                                \
  } while (0)
#define log_info_printf(format, ...)                                           \
  do {                                                                         \
    printf(format, __VA_ARGS__);                                               \
    fprintf(logger, format, __VA_ARGS__);                                      \
    fprintf(debug_logger, format, __VA_ARGS__);                                \
  } while (0)
#define log_debug_printf(format, ...)                                          \
  do {                                                                         \
    fprintf(debug_logger, format, __VA_ARGS__);                                \
  } while (0)