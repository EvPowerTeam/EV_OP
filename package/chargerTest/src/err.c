
#include "./include/API.h"
#include "./include/err.h"
#include <stdarg.h>
#include <syslog.h>

int  	daemon_proc = 0;	/* set nonzero by daemon_init() */
char	process_name[NAMELEN];
static void err_doit(int, int, const char *, va_list);

/* 
 * printf bubug infomation
 */
void 
log_msg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(1, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Nonfatal error related to system call
 * Print message and return */

void 
err_ret(const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	err_doit(1, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error related to system call
 * Print message and terminate */

void 
err_sys(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Fatal error related to system call
 * Print message, dump core, and terminate */

void 
err_dump(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
	abort();
	exit(1);
}

/* Nonfatal error unrelated to system call
 * Print message and return */

void 
err_msg(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	err_doit(0, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error unrelated to system call
 * Print message and terminate */

void 
err_quit(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	err_doit(0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Print message and return to caller
 * Caller specified "errnoflag" and "level" */

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
	int	errno_save, n;
	char	buf[MAXLINE+1];

	errno_save = errno;	/* value caller might want printed */
#ifndef  HAVE_VSPRINTF
	vsnprintf(buf, MAXLINE, fmt, ap); /* safe */
#else
	vsprintf(buf, fmt, ap);		/* not safe */
#endif
	n = strlen(buf);
	if (errnoflag)
		snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
	strcat(buf, "\n");

	if (daemon_proc){
		syslog(level, buf);
	} else {
		fflush(stdout);
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;	
}

