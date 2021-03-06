

#ifndef __NG_NG_H__
#define __NG_NG_H__

#define DEBUG_FILE_PATH "/tmp/debug_ev.log"

#undef EV_DEBUG
#if defined(DEBUG_CONSOLE) || defined(DEBUG_FILE) || defined(DEBUG_SYSLOG)
#define EV_DEBUG 1

#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

#endif

#include <time.h>
#include <string.h>

static inline char *ev_time_str()
{
	char *buff;
	time_t now;

	now = time(NULL);
	buff = ctime(&now);
	if (!buff)
		return NULL;

	if (buff[strlen(buff) - 1] == '\n')
		buff[strlen(buff) - 1] = '\0';

	return buff;
}

#if defined(DEBUG_CONSOLE)
#define debug_console(fmt, arg...) do {\
	char *_t = ev_time_str();\
	fprintf(stderr, "%s %s: " fmt "\n", _t, __FUNCTION__, ## arg);\
} while (0)
#else
#define debug_console(fmt, arg...) do { } while (0)
#endif

#if defined(DEBUG_FILE)
#define debug_file(fmt, arg...) do {\
	char *_t = ev_time_str();\
	FILE *file = fopen(DEBUG_FILE_PATH, "a");\
	fprintf(file, "%s [%i] %s: " fmt "\n", _t, getpid(), __FUNCTION__, ## arg);\
	fclose(file);\
} while(0)
#else
#define debug_file(fmt, arg...) do { } while (0)
#endif

#if defined(DEBUG_SYSLOG)
#define debug_syslog(fmt, arg...) do {\
	openlog("ev", LOG_PID, LOG_DAEMON | LOG_EMERG);\
	syslog(0, "%s: " fmt, __FUNCTION__, ## arg);\
	closelog();\
} while (0)
#else
#define debug_syslog(fmt, arg...) do { } while (0)
#endif


#if defined(NG_DEBUG)
#define debug_msg(fmt, arg...) do {			\
	debug_console(fmt, ## arg);			\
	debug_file(fmt, ## arg);			\
	debug_syslog(fmt, ## arg);			\
} while (0)
#else
#define debug_msg(fmt, arg...) do { } while (0)
#endif

#define NG_UNUSED(x) (x)__attribute__((unused))

#define DASHBOARD_SEP "### DASHBOARD_SEPARATOR"

#endif /* __NG_NG_H__ */
