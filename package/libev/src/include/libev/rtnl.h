
/***
 *
 * Copyright (C) 2014-2015 Open Mesh, Inc.
 *
 * The reproduction and distribution of this software without the written
 * consent of Open-Mesh, Inc. is prohibited by the United States Copyright
 * Act and international treaties.
 *
 ***/

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>

struct ev_rtnl_state {
	int fd;
	struct sockaddr_nl local;
	struct sockaddr_nl peer;
	uint32_t seq;
	uint32_t dump;
};

typedef int (*ev_rtnl_cb)(struct ev_rtnl_state *rtnl,
			  const struct sockaddr_nl *,
			  struct nlmsghdr *n, void *);

static inline uint32_t ev_rtnl_mgrp(uint32_t group)
{
	return group ? (1 << (group - 1)) : 0;
}

int ev_rtnl_open(struct ev_rtnl_state *rth, unsigned groups);
void ev_rtnl_close(struct ev_rtnl_state *rth);
int ev_rtnl_parse_attr(struct rtattr *tb[], int max, struct rtattr *rta,
			 int len);
int ev_rtnl_recv(struct ev_rtnl_state *rtnl, ev_rtnl_cb cb, void *arg);
int ev_rtnl_iface_link_set(const char *iface, bool up);
int ev_rtnl_iface_mtu_set(const char *iface, uint32_t mtu);
int ev_rtnl_iface_ip_add(const char *iface, uint32_t ip, uint32_t prefixlen);
int ev_rtnl_iface_ip_flush(const char *iface);
int ev_rtnl_iface_route_add(const char *iface, uint32_t dst, uint32_t prefixlen,
			    uint32_t gw, uint32_t table);
int ev_rtnl_iface_route_del(const char *iface, uint32_t dst, uint32_t prefixlen,
			    uint32_t gw, uint32_t table);
int ev_rtnl_iface_route_flush(const char *iface);
int ev_rtnl_bridge_create(const char *bridge);
int ev_rtnl_bridge_addif(const char *bridge, const char *slave);
int ev_rtnl_bridge_delif(const char *slave);
int ev_rtnl_vlan_add(const char *iface, uint16_t vid);
int ev_rtnl_iface_destroy(const char *iface);
int ev_rtnl_iface_rule_add(const char *iiface, uint32_t src, uint32_t dst,
			   uint32_t prefixlen, uint32_t table);
int ev_rtnl_iface_rule_del(const char *iiface, uint32_t src, uint32_t dst,
			   uint32_t prefixlen, uint32_t table);
bool ev_rtnl_route_has_default(const char *iface);
bool ev_rtnl_iface_link_get(const char *iface);
