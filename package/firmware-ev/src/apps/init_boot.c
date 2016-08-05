#include "init_boot.h"
#include "init_config.h"
#include <ev.h>
#include <libev/cmd.h>
#include <libev/uci.h>
#include <libev/file.h>
#include <libev/system.h>
#include <libev/const_strings.h>
#include <libev/msgqueue.h>

#include "init_config.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mqueue.h>

char *tmp_buff = NULL;
int tmp_buff_len = 100;


static void boot_setup_network()
{
	cmd_run("echo 1 > /proc/sys/net/ipv4/ip_forward");
	//bug
	cmd_run("for each in /proc/sys/net/ipv4/conf/* \
			do \
				echo 0 > $each/accept_redirects \
				echo 0 > $each/send_redirects \
			done");
	cmd_run("sysctl -p");
	cmd_run("stty -F /dev/ttyUSB0 9600 clocal cread cs8 -cstopb -parenb");
}

static void boot_init_dir(void)
{
    char    name[100];
    //mkdir(WORK_DIR, 0777);
    //sprintf(name, "%s/%s%c", WORK_DIR, CHAOBIAO_DIR, '\0');

    //mkdir(name, 0777);
    //sprintf(name, "%s/%s%c", WORK_DIR, CONFIG_DIR, '\0');
    //mkdir(name, 0777);
    //sprintf(name, "%s/%s%c", WORK_DIR, UPDATE_DIR, '\0');
    //mkdir(name, 0777);
    //sprintf(name, "%s/%s%c", WORK_DIR, RECORD_DIR, '\0');
    //mkdir(name, 0777);
}

int init_boot_boot(int EV_UNUSED(argc), char EV_UNUSED(**argv),
		 char EV_UNUSED(*extra_arg))
{
	debug_msg("module booting");
	init_config(0, NULL, NULL);
	boot_setup_network();
	boot_init_dir(); // 创建初始化目录
	//mqcreate(O_RDONLY | O_NONBLOCK, 10, 10, "/dashboard.checkin");
	mqcreate(O_RDONLY, 10, 100, "/dashboard.checkin");
	return 0;
}
