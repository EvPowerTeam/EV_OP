

#include "ev.h"
#include "apps.h"
#include <libev/process.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define EV_APP_NAME "ev"
#define FW_EV_VER_ID_SHORT "Beta 0.1"

static void print_version(void)
{
	printf("%s v2.0-%s\n", EV_APP_NAME, FW_EV_VER_ID_SHORT);
	printf("Copyright (C) Jaylin .\n");
}

static int print_avail_apps(void)
{
#if defined(NG_DEBUG)
	const struct ev_app **ev_app;

	printf("available applications:\n");

	for (ev_app = ev_apps; *ev_app; ++ev_app)
		printf("   * %s\n", (*ev_app)->name);
#endif

	return 1;
}

#if defined(DEBUG_TRACE)
static int check_trace(char *name)
{
	int pid, traced;

	switch(pid = fork()) {
	case 0:
		pid = getppid();
		traced = ptrace(PTRACE_ATTACH, pid, 0, 0);

		if (!traced) {
			process_send_signal(pid, SIGCONT);
			_exit(EXIT_SUCCESS);
		}

		perror(name);
		process_send_signal(pid, SIGKILL);
		goto err;
	case -1:
		break;
	default:
		if (pid == waitpid(pid, 0, 0))
			return EXIT_SUCCESS;

		break;
	}

	perror(name);
err:
	return -1;
}
#else
static int check_trace(char NG_UNUSED(*name))
{
	return 0;
}
#endif

int main(int argc, char **argv)
{
	const struct ev_app **ev_app;
	char *app_name, *ln_arg;
	int ret;

	ret = check_trace(argv[0]);
	if (ret < 0)
		return EXIT_FAILURE;

	if ((argc > 1) && (strlen(argv[1]) > 1) &&
	    (argv[1][0] == '-') && (argv[1][1] == 'v')) {
		print_version();
		return EXIT_SUCCESS;
	}

	app_name = strrchr(argv[0], '/');
	app_name = (app_name ? app_name + 1 : argv[0]);

	/* argument encoded in filename */
	ln_arg = strchr(app_name, '-');

	if (ln_arg) {
		*ln_arg = '\0';
		ln_arg++;
	}

	if (strncmp(app_name, EV_APP_NAME, APP_NAME_MAX) == 0) {
		if (argc == 1)
			return print_avail_apps();

		app_name = argv[1];
		argv++;
		argc--;
	}

	for (ev_app = ev_apps; *ev_app; ++ev_app)
		if (strncmp(app_name, (*ev_app)->name, APP_NAME_MAX) == 0)
			break;

	if (!(*ev_app)) {
		printf("Error - the application '%s' was not found\n", app_name);
		return EXIT_FAILURE;
	}

	if (argc < 2) {
		if ((*ev_app)->dflt)
			return (*ev_app)->dflt(argc - 1, argv + 1, ln_arg);

		printf("Error - not enough arguments to run %s\n",
		       app_name);
		goto err_param;
	}

	if ((strcmp(argv[1], "boot") == 0) && (*ev_app)->boot)
		return (*ev_app)->boot(argc - 2, argv + 2);

	if ((strcmp(argv[1], "start") == 0) && (*ev_app)->start)
		return (*ev_app)->start(argc - 2, argv + 2);

	if ((strcmp(argv[1], "stop") == 0) && (*ev_app)->stop)
		return (*ev_app)->stop(argc - 2, argv + 2);

	if ((strcmp(argv[1], "restart") == 0) && (*ev_app)->restart)
		return (*ev_app)->restart(argc - 2, argv + 2);

	if ((*ev_app)->dflt)
		return (*ev_app)->dflt(argc - 1, argv + 1, ln_arg);

	printf("Error - unknown parameter: %s\n", argv[1]);

err_param:
	printf("Use one of the following parameters:\n");
	if ((*ev_app)->boot)
		printf("   * boot\n");
	if ((*ev_app)->start)
		printf("   * start\n");
	if ((*ev_app)->stop)
		printf("   * stop\n");
	if ((*ev_app)->restart)
		printf("   * restart\n");

	return EXIT_FAILURE;
}
