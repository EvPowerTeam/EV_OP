#include "init_boot.h"
#include "init_config.h"
#include <ev.h>
#include <libev/cmd.h>
#include <libev/uci.h>
#include <libev/file.h>
#include <libev/system.h>
#include <libev/const_strings.h>
#include <libev/msgqueue.h>

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

static void  boot_init_dir(void)
{
	char name[100];
	mkdir(WORK_DIR, 0711);
	sprintf(name, "%s/%s", WORK_DIR, CHAOBIAO_DIR);
	//创建抄表目录
	mkdir(name, 0711);

	sprintf(name, "%s/%s", WORK_DIR, CONFIG_DIR);
	mkdir(name, 0711);

	sprintf(name, "%s/%s", WORK_DIR, UPDATE_DIR);
	mkdir(name, 0711);

	sprintf(name, "%s/%s", WORK_DIR, RECORD_DIR);
	mkdir(name, 0711);

	sprintf(name, "%s/%s", WORK_DIR, EXCEPTION_DIR);
	mkdir(name, 0711);

	sprintf(name, "%s/%s", WORK_DIR, LOG_DIR);
	mkdir(name, 0777);
    
	mkdir(ROUTER_LOG_DIR, 0777);
}

int init_boot_boot(int EV_UNUSED(argc), char EV_UNUSED(**argv),
		 char EV_UNUSED(*extra_arg))
{
	int ret;

	debug_syslog("module booting");
	if (!file_exists(path_router_reboot_log))
		file_trunc_to_zero(path_router_reboot_log);
	if (file_size(path_router_reboot_log) > 32768)
		file_trunc_to_zero(path_router_reboot_log);

	cmd_frun("echo \"`date` Router reboot\" >> %s", path_router_reboot_log);
	//boot_setup_network();
	boot_init_dir(); // 创建初始化目录
	init_config(0, NULL, NULL);
        if (file_exists(path_config_charger))
		file_delete(path_config_charger);
	//mqcreate(O_RDONLY | O_NONBLOCK, 10, 10, "/dashboard.checkin");
	ret = mqcreate(O_EXCL, 10, 100, "/dashboard.checkin");
	debug_msg("checkin queue ret: %d errno: %d", ret, errno);
	sleep(2);
	ret = mqcreate(O_EXCL, 10, 200, "/server.cmd");
	debug_msg("command queue ret: %d errno: %d", ret, errno);
	return ret;
}
