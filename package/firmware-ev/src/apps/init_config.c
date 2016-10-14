


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


const char path_crashlog_debugfs[] = "/sys/kernel/debug/crashlog";
const char path_crashlog_tmpfs[] = "/tmp/crashlog";
const char path_udisk[] = "/mnt/umemory";
const char path_xl2tpd[] = "/etc/xl2tpd/xl2tpd.conf";
const char path_strongswan[] = "/etc/strongswan.conf";
const char path_ipsec_conf[] = "/etc/ipsec.conf";
const char path_ipsec_key[] = "/etc/ipsec.secrets";
const char path_ppp[] = "/etc/ppp/hk.options.xl2tpd.client";


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
	if (file_exists("/root/root"))
		cmd_frun(cmd_cp, "/root/root", "/etc/crontabs/root");
	cmd_run("mwan3 stop");
	cmd_run("./etc/init.d/cron restart");
	cmd_run("chmod 777 /root/setup_vpn.sh");
	cmd_run("chmod 777 /root/vpn_monitor");
}

/**
 * save configuration to uci
 * */
static void init_router_config()
{
	debug_msg("router setup");
	file_trunc_to_zero(path_udpserver_pid);
	if (file_exists(path_config_charger))
		file_delete(path_config_charger);
	//router id
	//ev_uci_save_val_int();
	//vpn IP
}

int init_config(int EV_UNUSED(argc), char EV_UNUSED(**argv), char EV_UNUSED(*extra_arg))
{
	debug_msg("configuration initlization");
	init_vpn_config();
	init_router_config();
	return 0;
}
