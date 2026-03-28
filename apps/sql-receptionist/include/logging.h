#include <stdio.h>

extern FILE *logger;
extern FILE *debug_logger;

extern void log_critical(const char *message);
extern void log_error(const char *message);
extern void log_warn(const char *message);
extern void log_info(const char *message);
extern void log_debug(const char *message);

extern void log_critical_printf(const char *format, ...);
extern void log_error_printf(const char *format, ...);
extern void log_warn_printf(const char *format, ...);
extern void log_info_printf(const char *format, ...);
extern void log_debug_printf(const char *format, ...);