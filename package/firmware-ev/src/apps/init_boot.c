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
	cmd_run("for each in /proc/sys/net/ipv4/conf/* \
			do \
				echo 0 > $each/accept_redirects \
				echo 0 > $each/send_redirects \
			done");
	cmd_run("sysctl -p");

}

static void boot_init_dir(void)
{
    //char    name[100];
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
	int ret;

	debug_syslog("module booting");
	cmd_run("echo \"`date` \"+\": reboot EV Router\" >> /mnt/umemory/routerlog/`date \"+%Y-%m-%d\"`.log");
	init_config(0, NULL, NULL);
	cmd_run("sh /root/setup_sangfor start");
	//boot_setup_network();
	//boot_init_dir(); // 创建初始化目录
	//mqcreate(O_RDONLY | O_NONBLOCK, 10, 10, "/dashboard.checkin");
	ret = mqcreate(O_EXCL, 10, 100, "/dashboard.checkin");
	debug_msg("ret: %d errno: %d", ret, errno);
	sleep(2);
	ret = mqcreate(O_EXCL, 10, 200, "/server.cmd");
	debug_msg("ret: %d errno: %d", ret, errno);
	cmd_run("mwan3 stop");
	return 0;
}
