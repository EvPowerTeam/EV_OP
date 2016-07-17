
/***
 *
 * Copyright (C) 2016 EV Power Ltd, Inc.
 * Author: Jaylin Yu
 *
 ***/

#include "init_vpn.h"

#include <ev.h>
#include <libev/cmd.h>
#include <libev/file.h>
#include <libev/system.h>
#include <libev/const_strings.h>
#include <libev/uci.h>
#include <libev/system.h>

#include <stdlib.h>
#include <stdio.h>


int init_vpn_check(int NG_UNUSED(argc), char NG_UNUSED(**argv),
		 char NG_UNUSED(*extra_arg))
{
	debug_msg("checkin vpn status");
	//for periodic checking vpn with full path
	cmd_frun("/bin/sh /root/vpn_monitor");

}

int init_vpn_up(int NG_UNUSED(argc), char NG_UNUSED(**argv),
		 char NG_UNUSED(*extra_arg))
{
	debug_msg("checkin vpn status");
	//for periodic checking vpn with full path
	cmd_frun("/bin/sh /root/vpn_monitor");

}

int init_vpn_down(int NG_UNUSED(argc), char NG_UNUSED(**argv),
		 char NG_UNUSED(*extra_arg))
{
	debug_msg("stoping ipsec");
	//for periodic checking vpn with full path
	cmd_frun("./etc/init.d/xl2tpd stop");
	cmd_frun("ipsec stop");

}
