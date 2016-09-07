
/***
 *
 * Copyright (C) 2009-2015 EV Power Ltd, Inc.
 *
 ***/

#include "include/dashboard.h"
#include "dashboard_controller.h"
#include <libev/cmd.h>
#include <libev/file.h>
#include <libev/system.h>
#include <libev/process.h>
#include <libev/uci.h>
#include <libev/const_strings.h>
#include "dashboard_checkin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>

/* bitmask of dshbrd_checkin_parts to report in the next checkin */
uint8_t dshbrd_checkin_parts;

#define DSH_CTRL_PIPE "/tmp/dashboard_controller.pipe"

#define ORPHAN_MAX_CHECKINS   6
const char path_checkin_counter[] = "/tmp/checkin_counter";

static char pid_path_buff[50];
static char checkin_stopped = 0;
static bool checkin_pending;

static int checkin_interval, pa_per_checkin;
struct timespec timeout;


static void kill_running_checkin(void)
{
	int child_pid = file_read_int(path_checkin_pid);

	if (child_pid == 0)
		return;

	process_send_signal(child_pid, SIGTERM);
	wait(NULL);
	file_delete(path_checkin_pid);
}

static void dashboard_checkin_terminated(void)
{
	file_delete(path_checkin_pid);
}

static void signal_handler(int sig)
{
	int ret;

	switch (sig) {
	case SIGTERM:
		checkin_stopped = 1;
		kill_running_checkin();
		break;
	case SIGSEGV:
		ret = ev_uci_data_get_val(pid_path_buff, sizeof(pid_path_buff),
					  uci_addr_dshbrd_controller_pid);
		if (ret != 0)
			break;
		file_delete(pid_path_buff);
		break;
	}
}

int checkin_interval_get(void)
{
	char interval_str[10];
	int ret, interval = 5 * 60;

	ret = ev_uci_data_get_val(interval_str, sizeof(interval_str),
				    uci_addr_dshbrd_checkin_interval);
	if (ret != 0)
		goto out;

	interval = strtol(interval_str, (char **)NULL, 10);
	if (interval < 60)
		interval = 60;

out:
	return interval;
}

int pa_interval_get(int checkin_interval)
{
	int ret, interval, off;
	char uploads_num_str[10], enabled[2];

	pa_per_checkin = 5;

	ret = ev_uci_data_get_val(enabled, sizeof(enabled),
				  uci_addr_presence_enable);
	if ((ret != 0) || (strlen(enabled) < 1) || (enabled[0] == '0')) {
		/* prequestd is disabled - no need to send PA data */
		pa_per_checkin = 0;
		return checkin_interval;
	}

	ret = ev_uci_data_get_val(uploads_num_str, sizeof(uploads_num_str),
				  uci_addr_presence_uploads_per_checkin);
	if (ret == 0) {
		pa_per_checkin = strtol(uploads_num_str, (char **)NULL, 10);
		if (pa_per_checkin < 1)
			pa_per_checkin = 1;
	}

	interval = checkin_interval / pa_per_checkin;
	/* interval *must* be a multiple of 5 and can't be smaller than 10
	 * seconds
	 */
	if (interval < 10)
		interval = 10;
	/* round up to the next multiple of 5 */
	off = interval % 5;
	if (off != 0)
		interval += 5 - off;

	return interval;
}


#if defined(DASH_DEBUG)
static void dashboard_checkin_run(const char *msg)
#else
static void dashboard_checkin_run(const char DASH_UNUSED(*msg))
#endif
{
	int child_pid;

	/* no concurrent checkins! */
	if (!file_exists(path_checkin_pid))
		goto out;

	child_pid = file_read_int(path_checkin_pid);

	if (!process_is_alive(child_pid)) {
		/* if the child is not alive it means it crashed!
		 * Reset the controller state and allow it to spawn a
		 * new checkin process
		 */
		sys_logger(CHECKIN_NAME, "dead process detected. Resetting.");
		dashboard_checkin_terminated();
	} else if (process_is_alive(child_pid)) {
		debug_msg("checkin in progress. Postponing request..");
		return;
	}
out:
	checkin_pending = false;
	child_pid = process_create_child(dashboard_checkin,
					 &dshbrd_checkin_parts);
	if (child_pid < 0) {
		debug_msg("cannot create checkin child process\n");
		return;
	}
	file_write_int(child_pid, path_checkin_pid);

	debug_msg("%s", msg);
}

/**
 * dasbrd_load_intervals - loads various intervals
 *
 * Loads the various intervals and compute main loop interval
 */
static void dashboard_load_intervals(void)
{
	int interval;

	/* randomize check-in */
	interval = checkin_interval_get();

	/* compute the real checkin interval based on the configured number of
	 * uploads per (standard) checkin
	 */
	checkin_interval = pa_interval_get(interval);
}

static void dashboard_checkin_parse_cmd(uint8_t cmd)
{
	switch (cmd) {
	case CMD_CHECKIN:
		checkin_pending = true;
		dshbrd_checkin_parts = CHECKIN_CT;
		if (pa_per_checkin > 0)
			dshbrd_checkin_parts |= CHECKIN_PA;
		dashboard_checkin_run("starting checkin on demand");
		break;
	case CMD_RELOAD:
		dashboard_load_intervals();
		break;
	case CMD_RELEASE:
		dashboard_checkin_terminated();
		break;
	default:
		debug_msg("unknown command %#.2hhx", cmd);
		break;
	}
}

int dshbrd_controller_start(int DASH_UNUSED(argc), char DASH_UNUSED(**argv))
{
	int ret, checkin_counter, s, res = EXIT_FAILURE, fd;
	int interval, pa_count;
	sigset_t emptyset;
	struct sigaction sa;
	struct timespec now, last_checkin_time;
	uint8_t cmd;
	fd_set fds;
	debug_msg("here1");
	
	ret = ev_uci_data_get_val(pid_path_buff, sizeof(pid_path_buff),
				    uci_addr_dshbrd_controller_pid);
	if (ret != 0)
		goto err;

	ret = process_is_alive(file_read_int(pid_path_buff));
	if (ret == 1)
		goto err;

	process_daemonize();
	file_create_dir(pid_path_buff);
	file_write_int(getpid(), pid_path_buff);

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_NOCLDWAIT;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		debug_msg("dashboard signal failure");
	}

	sa.sa_handler = signal_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigemptyset(&emptyset);

	if (mkfifo(DSH_CTRL_PIPE, S_IRWXU) < 0) {
		if (errno != EEXIST) {
			debug_msg("cannot create pipe: %s", strerror(errno));
			goto err;
		}
	}

	fd = open(DSH_CTRL_PIPE, O_RDWR);
	if (fd < 0) {
		debug_msg("cannot open pipe: %s", strerror(errno));
		goto err_unlink;
	}


	dashboard_load_intervals();

	/* first fast checkin */
	dshbrd_checkin_parts = CHECKIN_CT;
	if (pa_per_checkin > 0)
		dshbrd_checkin_parts |= CHECKIN_PA;
	dashboard_checkin_run("running first checkin");
	clock_gettime(CLOCK_MONOTONIC, &last_checkin_time);

	srandom(getpid());
	timeout.tv_sec = random() % (checkin_interval * 2);
	pa_count = 0;

	while (1) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		s = pselect(fd + 1, &fds, NULL, NULL, &timeout, &emptyset);

		clock_gettime(CLOCK_MONOTONIC, &now);
		interval = (now.tv_sec - last_checkin_time.tv_sec);

		if (s < 0) {
			if (errno != EINTR) {
				debug_msg("error during select(): %s",
					  strerror(errno));
				goto err_close;
			}
			if (checkin_stopped) {
				debug_msg("dashboard_controller stopped.");
				break;
			}

			/* check if a checkin was pending and possibly start it */
			if (checkin_pending)
				dashboard_checkin_run("starting delayed checkin");

			if (interval >= timeout.tv_sec)
				timeout.tv_sec = 1;
			else
				timeout.tv_sec -= interval;
			continue;
		} else if (s > 0) {
			if (!FD_ISSET(fd, &fds))
				continue;

			ret = read(fd, &cmd, 1);
			if (ret < 0) {
				debug_msg("read() failed: %s", strerror(errno));
				goto err_close;
			}
			if (ret == 0) {
				debug_msg("read() returned 0");
				continue;
			}

			debug_msg("received message = '%#.2hhx'", cmd);
			dashboard_checkin_parse_cmd(cmd);

			/* check if the checkin interval was changed */
			if (checkin_interval != timeout.tv_sec) {
				timeout.tv_sec = checkin_interval;
				continue;
			}

			if (interval >= timeout.tv_sec)
				timeout.tv_sec = 1;
			else
				timeout.tv_sec -= interval;
			continue;
		}

		dshbrd_checkin_parts = CHECKIN_EMPTY;

		/* If prequestd is enabled, the pselect timeout is tuned to the
		 * PA checkin interval, therefore every time the pselect times
		 * out a new PA checkin has to be performed.
		 */
		if (pa_per_checkin > 0) {
			pa_count++;
			dshbrd_checkin_parts |= CHECKIN_PA;
		}

		/* check if it is time to send the full checkin.
		 * this is always true when prequestd is disabled
		 */
		if ((pa_per_checkin == 0) || (pa_count == pa_per_checkin)) {
			dshbrd_checkin_parts |= CHECKIN_CT;
			pa_count = 0;
		}

		dashboard_checkin_run("start period checkin");

		clock_gettime(CLOCK_MONOTONIC, &last_checkin_time);
		timeout.tv_sec = checkin_interval;

		sys_set_orphan_mode();
		if ((sys_orphan_mode == ORPHAN_CLIENT) &&
		    !ev_uci_check_val("1", uci_addr_upgrade_ready)) {
			checkin_counter = file_read_int(path_checkin_counter);
			checkin_counter++;
			/* if the node did not reboot on its own until now it
			 * means that it entered orphan mode due to a network
			 * outage. There is no need to keep this state anymore.
			 * Reboot!
			 */
			if (checkin_counter == ORPHAN_MAX_CHECKINS) {
				file_write_string(path_reboot_reason,
						  "max checkin reached in orphan mode");
				sync();
				cmd_run(cmd_reboot);
				break;
			}

			file_write_int(checkin_counter, path_checkin_counter);
		}
		debug_msg("going to sleep after periodic checkin");
	}

	res = EXIT_SUCCESS;
err_close:
	close(fd);
err_unlink:
	unlink(DSH_CTRL_PIPE);
err:
	return res;
}

int dshbrd_controller_stop(int DASH_UNUSED(argc), char DASH_UNUSED(**argv))
{
	int ret;

	ret = ev_uci_data_get_val(pid_path_buff, sizeof(pid_path_buff),
				    uci_addr_dshbrd_controller_pid);
	if (ret != 0)
		goto err;

	ret = process_is_alive(file_read_int(pid_path_buff));
	if (ret == 0)
		goto out;

	ret = process_send_signal(file_read_int(pid_path_buff), SIGTERM);
	file_delete(pid_path_buff);

	return ret;
out:
	return EXIT_SUCCESS;

err:
	return EXIT_FAILURE;
}

int dshbrd_controller_send_msg(uint8_t cmd)
{
	int fd;

	fd = open(DSH_CTRL_PIPE, O_WRONLY);
	if (fd < 0) {
		debug_msg("cannot talk to controller: %s",
			  strerror(errno));
		return -1;
	}

	/* send the command to the dashboard controller */
	if (write(fd, &cmd, 1) < 0)
		debug_msg("cannot send command to controller: %s",
			  strerror(errno));

	close(fd);

	return 0;
}

int dshbrd_controller_checkin(int DASH_UNUSED(argc), char DASH_UNUSED(**argv))
{
	if (dshbrd_controller_send_msg(CMD_CHECKIN) < 0) {
		debug_msg("error while sending CHECKIN cmd to controller");
		return EXIT_FAILURE;
	}
	debug_msg("CHECKIN cmd sent to controller");
	return EXIT_SUCCESS;
}

int dshbrd_controller_reload(int DASH_UNUSED(argc), char DASH_UNUSED(**argv))
{
	if (dshbrd_controller_send_msg(CMD_RELOAD) < 0) {
		debug_msg("error while sending RELOAD cmd to controller");
		return EXIT_FAILURE;
	}
	debug_msg("RELOAD cmd sent to controller");
	return EXIT_SUCCESS;
}
