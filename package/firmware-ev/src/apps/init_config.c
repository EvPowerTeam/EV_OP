#include "init_boot.h"
#include "init_config.h"
#include <ev.h>
#include <libev/cmd.h>
#include <libev/uci.h>
#include <libev/file.h>
#include <libev/system.h>
#include <libev/version.h>
#include <libev/const_strings.h>

//#include "hw_detect.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/if_ether.h>

/*
 * For now we are using strongswan 5.3.2 with Xl2tpd
 * */
static void init_vpn_config()
{
	if (file_exists("/root/strongswan.conf"))
		cmd_frun(cmd_cp, "/root/strongswan.conf", path_strongswan);
	if (file_exists("/root/ipsec.conf"))
		cmd_frun(cmd_cp, "/root/ipsec.conf", path_ipsec_conf);
	if (file_exists("/root/ipsec.secrets"))
		cmd_frun(cmd_cp, "/root/ipsec.secrets", path_ipsec_key);
	if (file_exists("/root/xl2tpd.conf"))
		cmd_frun(cmd_cp, "/root/xl2tpd.conf", path_xl2tpd);
	if (file_exists("/root/hk.options.xl2tpd.client"))
		cmd_frun(cmd_cp, "/root/hk.options.xl2tpd.client", path_ppp);
	if (file_exists("/root/9qu.xl2tpd"))
		cmd_frun(cmd_cp, "/root/9qu.xl2tpd", "/etc/ppp/");
	if (file_exists("/root/root"))
		cmd_frun(cmd_cp, "/root/root", "/etc/crontabs/root");

	cmd_run("./etc/init.d/cron restart");
	cmd_run("chmod 777 /root/setup_vpn.sh");
	cmd_run("chmod 777 /root/setup_9qu.sh");
	cmd_run("chmod 777 /root/vpn_monitor");
	cmd_run("chmod 777 /root/vpn_monitor_9qu");
	cmd_run("chmod 777 /root/if.sh");
}

static void  init_dir(void)
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

/**
 * save configuration to uci
 * */
static void init_router_config()
{
	debug_msg("router setup");
	file_trunc_to_zero(path_udpserver_pid);
	//router id
	//ev_uci_save_val_int();
	//vpn IP
}

int init_config(int EV_UNUSED(argc), char EV_UNUSED(**argv), char EV_UNUSED(*extra_arg))
{
	debug_msg("configuration initlization");
	init_vpn_config();
	init_router_config();
	init_dir(); // 创建初始化目录
	return 0;
}
