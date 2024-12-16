#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "logging.h"

#ifdef DEBUG
int logging_level_ = LOG_DEBUG;
#else
int logging_level_ = LOG_INFO;
#endif

static void log_set_modestr(int mode, char *modestr)
{
	if (isatty(STDERR_FILENO)) {
		switch (mode) {
		case LOG_DEBUG:
			strcpy(modestr, "\x1b[1;34mDEBUG\x1b[0m");
			break;
		case LOG_INFO:
			strcpy(modestr, "\x1b[1;36mINFO\x1b[0m ");
			break;
		case LOG_WARN:
			strcpy(modestr, "\x1b[1;33mWARN\x1b[0m ");
			break;
		case LOG_ERROR:
			strcpy(modestr, "\x1b[1;31mERROR\x1b[0m");
			break;
		case LOG_FATAL:
			strcpy(modestr, "\x1b[1;35mFATAL\x1b[0m");
			break;
		}
	} else {
		switch (mode) {
		case LOG_DEBUG:
			strcpy(modestr, "DEBUG");
			break;
		case LOG_INFO:
			strcpy(modestr, "INFO ");
			break;
		case LOG_WARN:
			strcpy(modestr, "WARN ");
			break;
		case LOG_ERROR:
			strcpy(modestr, "ERROR");
			break;
		}
	}
}

inline void log_set_level(int level)
{
	logging_level_ = level;
}

void log_va(int mode, const char *file, int line, const char *fmt, va_list ap)
{
	time_t rn = time(NULL);
	struct tm *local = localtime(&rn);
	char timestamp[32];
	char modestr[32] = "";

	log_set_modestr(mode, modestr);
	strftime(timestamp, sizeof(timestamp), "%H:%M:%S", local);
	fprintf(stderr, "%s %s:%d %s ", timestamp, file, line, modestr);
	vfprintf(stderr, fmt, ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
}

void log_force_raw(int mode, const char *file, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_va(mode, file, line, fmt, ap);
	va_end(ap);
}

void log_raw(int mode, const char *file, int line, const char *fmt, ...)
{
	va_list ap;

	if (mode < logging_level_)
		return;

	va_start(ap, fmt);
	log_va(mode, file, line, fmt, ap);
	va_end(ap);
}
