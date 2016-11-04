
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


int init_vpn_check(int EV_UNUSED(argc), char EV_UNUSED(**argv),
		 char EV_UNUSED(*extra_arg))
{
	debug_msg("checkin vpn status");
	//for periodic checking vpn with full path
	cmd_frun("/bin/sh /root/vpn_monitor");
#if 0 
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "ppp1" */
	strncpy(ifr.ifr_name, "ppp1", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	/* display result */
	debug_msg("ppp1 :%s\n",
		  inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	
	/* I want IP address attached to "ppp2" */
	strncpy(ifr.ifr_name, "ppp2", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	/* display result */
	debug_msg("ppp2 :%s\n",
		  inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
#endif
	return 0;
}

int init_vpn_up(int EV_UNUSED(argc), char EV_UNUSED(**argv),
		 char EV_UNUSED(*extra_arg))
{
	debug_msg("checkin vpn status");
	//for periodic checking vpn with full path
	cmd_frun("/bin/sh /root/vpn_monitor");
	return 0;
}

int init_vpn_down(int EV_UNUSED(argc), char EV_UNUSED(**argv),
		 char EV_UNUSED(*extra_arg))
{
	debug_msg("stoping ipsec");
	//for periodic checking vpn with full path
	cmd_frun("./etc/init.d/xl2tpd stop");
	cmd_frun("ipsec stop");
	return 0;
}
