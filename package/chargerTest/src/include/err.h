
#ifndef __ERR__H
#define	__ERR__H


#define MAXLINE		4096
#define	BUFFSIZE	8192
#define NAMELEN		100
/* process name */
extern char process_name[];
extern int daemon_proc;

/*  output debug infomation to console or 
 *  log file  */
void log_msg(const char *fmt, ...);

/* Nonfatal error related to system call
 * Print message and return */
void err_ret(const char *fmt, ...);


/* Fatal error related to system call
 * Print message and terminate */
void err_sys(const char *fmt, ...);

/* Fatal error related to system call
 * Print message, dump core, and terminate */
void err_dump(const char *fmt, ...);

/* Nonfatal error unrelated to system call
 * Print message and return */
void err_msg(const char *fmt, ...);

/* Fatal error unrelated to system call
 * Print message and terminate */
void err_quit(const char *fmt, ...);


#define _GNU_SOURCE
#define DEBUG_FILE_PATH "/tmp/debug_charger.log"

#undef LIBNG_DEBUG
#if defined(DEBUG_CONSOLE) || defined(DEBUG_FILE) || defined(DEBUG_SYSLOG)
#define LIBNG_DEBUG 1

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>

static inline char *ng_time_str()
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

#endif

#if defined(DEBUG_CONSOLE)
#define debug_console(fmt, arg...) do {\
	char *_t = ng_time_str();\
	fprintf(stderr, "%s %s: " fmt "\n", _t, __FUNCTION__, ## arg);\
} while (0)
#else
#define debug_console(fmt, arg...) do { } while (0)
#endif

#if defined(DEBUG_FILE)
#define debug_file(fmt, arg...) do {\
	char *_t = ng_time_str();\
	FILE *file = fopen(DEBUG_FILE_PATH, "a");\
	fprintf(file, "%s [%i] %s: " fmt "\n", _t, getpid(), __FUNCTION__, ## arg);\
	fclose(file);\
} while(0)
#else
#define debug_file(fmt, arg...) do { } while (0)
#endif

#if defined(DEBUG_SYSLOG)
#define debug_syslog(fmt, arg...) do {\
	openlog("libng", LOG_PID, LOG_DAEMON | LOG_EMERG);\
	syslog(0, "%s: " fmt, __FUNCTION__, ## arg);\
	closelog();\
} while (0)
#else
#define debug_syslog(fmt, arg...) do { } while (0)
#endif


#if defined(LIBNG_DEBUG)
#define debug_msg(fmt, arg...) do {			\
	debug_console(fmt, ## arg);			\
	debug_file(fmt, ## arg);			\
	debug_syslog(fmt, ## arg);			\
} while (0)
#else
#define debug_msg(fmt, arg...) do { } while (0)
#endif

#define NG_UNUSED(x) (x)__attribute__((unused))
#define CERTS_PATH "/etc/ssl/certs/"


#endif // err.h

