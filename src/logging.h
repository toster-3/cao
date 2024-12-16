#ifndef LOGGING_H_
#define LOGGING_H_

#define LOG_DEBUG 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERROR 4
#define LOG_FATAL 5

#define log_log(mode, ...) log_raw((mode), __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_raw(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) log_raw(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_raw(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_raw(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_raw(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/* show log no matter the level */
#define log_force(mode, ...)                                                   \
	log_force_raw((mode), __FILE__, __LINE__, __VA_ARGS__)

extern int logging_level_;

void log_level_raw(int, int, const char *, int, const char *, ...);

void log_raw(int, const char *, int, const char *, ...);

void log_set_level(int);

#endif
