#include "init_boot.h"
#include "init_config.h"
#include <ev.h>
#include <libev/cmd.h>
#include <libev/uci.h>
#include <libev/file.h>
#include <libev/system.h>
#include <libev/const_strings.h>

#include "init_config.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

char *tmp_buff = NULL;
int tmp_buff_len = 100;


static void boot_setup_network()
{
	cmd_run("echo 1 > /proc/sys/net/ipv4/ip_forward");
	cmd_run("for each in /proc/sys/net/ipv4/conf/* \
			do \
				echo 0 > $each/accept_redirects \
				echo 0 > $each/send_redirects \
			done");
	cmd_run("sysctl -p");

}

int init_boot_boot(int NG_UNUSED(argc), char NG_UNUSED(**argv),
		 char NG_UNUSED(*extra_arg))
{
	debug_msg("module booting");
	init_config(NULL, NULL, NULL);
	boot_setup_network();

}
