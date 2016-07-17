
/***
 *
 * Copyright (C) 2014-2015
 *
 ***/

#include "libev.h"

#include <libev/rtnl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/uio.h>
#include <net/if.h>

#include <linux/fib_rules.h>


struct ev_rtnl_link_req {
	struct nlmsghdr n;
	struct ifinfomsg i;
	char buf[256];
};

struct ev_rtnl_addr_req {
	struct nlmsghdr n;
	struct ifaddrmsg i;
	char buf[256];
};

struct ev_rtnl_route_req {
	struct nlmsghdr n;
	struct rtmsg r;
	char buf[256];
};

typedef int (*route_cb_t)(struct ev_rtnl_state *, struct rtmsg *,
			  struct rtattr **, unsigned int, void *);

struct ev_rtnl_router_cb_data {
	void *arg;
	unsigned int ifindex;
	route_cb_t cb;
};

static int rcvbuf = 1024 * 4;

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *)(((void *)(nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#define NG_ADDATTR(_msg, _max_size, _attr, _data, _size)			\
	{									\
		if (ev_addattr_l(_msg, _max_size, _attr, _data, _size) < 0)	\
			goto err;						\
	}

static int ev_addattr_l(struct nlmsghdr *n, int maxlen, int type,
			const void *data, int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if ((int)(NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len)) > maxlen) {
		debug_msg("error: message exceeded bound of %d", maxlen);
		return -1;
	}

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);

	return 0;
}

void ev_rtnl_close(struct ev_rtnl_state *rtnl)
{
	if (rtnl->fd < 0)
		return;

	close(rtnl->fd);
	rtnl->fd = -1;
}

int ev_rtnl_open(struct ev_rtnl_state *rtnl, uint32_t groups)
{
	socklen_t addr_len;
	int sndbuf = 1024 * 2;

	memset(rtnl, 0, sizeof(*rtnl));

	rtnl->fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (rtnl->fd < 0) {
		debug_msg("cannot open netlink socket");
		return -1;
	}

	if (setsockopt(rtnl->fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0) {
		debug_msg("SO_SNDBUF: %s", strerror(errno));
		return -1;
	}

	if (setsockopt(rtnl->fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0) {
		debug_msg("SO_RCVBUF: %s", strerror(errno));
		return -1;
	}

	memset(&rtnl->local, 0, sizeof(rtnl->local));
	rtnl->local.nl_family = AF_NETLINK;
	rtnl->local.nl_groups = groups;

	if (bind(rtnl->fd, (struct sockaddr*)&rtnl->local, sizeof(rtnl->local)) < 0) {
		debug_msg("cannot bind netlink socket: %s", strerror(errno));
		return -1;
	}

	addr_len = sizeof(rtnl->local);
	if (getsockname(rtnl->fd, (struct sockaddr*)&rtnl->local, &addr_len) < 0) {
		debug_msg("cannot getsockname: %s", strerror(errno));
		return -1;
	}
	if (addr_len != sizeof(rtnl->local)) {
		debug_msg("wrong address length %d", addr_len);
		return -1;
	}

	if (rtnl->local.nl_family != AF_NETLINK) {
		debug_msg("wrong address family %d", rtnl->local.nl_family);
		return -1;
	}

	rtnl->seq = time(NULL);

	return 0;
}

int ev_rtnl_recv(struct ev_rtnl_state *rtnl, ev_rtnl_cb cb, void *arg)
{
	int msg_len, payload_len, err, status;
	struct nlmsghdr *h;
	struct sockaddr_nl nladdr;
	struct iovec iov;
	char buf[8192];
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;

	iov.iov_base = buf;
rtnl_recv:
	iov.iov_len = sizeof(buf);
	status = recvmsg(rtnl->fd, &msg, 0);

	if (status < 0) {
		if (errno == EINTR || errno == EAGAIN)
			goto rtnl_recv;

		debug_msg("rtnl receive error %s (%d)", strerror(errno),
			  errno);

		return -1;
	}

	if (status == 0) {
		debug_msg("eof from rtnl");
		return -1;
	}

	if (msg.msg_namelen != sizeof(nladdr)) {
		debug_msg("mismatching sender address length == %d",
			  msg.msg_namelen);
		return -1;
	}

	h = (struct nlmsghdr *)buf;
	while (status >= (int)sizeof(*h)) {
		msg_len = h->nlmsg_len;
		payload_len = msg_len - sizeof(*h);

		if (payload_len < 0 || msg_len > status) {
			if (msg.msg_flags & MSG_TRUNC) {
				debug_msg("message truncated");
				return -1;
			}

			debug_msg("received malformed message");
			return -1;
		}

		err = cb(rtnl, &nladdr, h, arg);
		if (err < 0)
			return err;

		status -= NLMSG_ALIGN(msg_len);
		h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(msg_len));
	}

	if (msg.msg_flags & MSG_TRUNC) {
		debug_msg("message truncated");
		return -1;
	}

	if (status) {
		debug_msg("%i bytes not parsed", status);
		return -1;
	}

	return 0;
}

int ev_rtnl_parse_attr(struct rtattr *tb[], int max, struct rtattr *rta,
		       int len)
{
	uint16_t type;

	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
	while (RTA_OK(rta, len)) {
		type = rta->rta_type;
		if ((type <= max) && (!tb[type]))
			tb[type] = rta;
		rta = RTA_NEXT(rta,len);
	}

	if (len)
		debug_msg("not parsed bytes %d, rta_len=%d", len,
			  rta->rta_len);

	return 0;
}

static int ev_rtnl_send(struct ev_rtnl_state *rtnl, struct nlmsghdr *n,
			pid_t peer, unsigned int groups,
			struct nlmsghdr *answer)
{
	struct sockaddr_nl nladdr;
	int status, len, rem_len;
	struct nlmsgerr *err;
	struct nlmsghdr *h;
	unsigned int seq;
	char buf[16384];
	struct iovec iov = {
		.iov_base = (void*) n,
		.iov_len = n->nlmsg_len
	};
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = peer;
	nladdr.nl_groups = groups;

	n->nlmsg_seq = seq = ++rtnl->seq;

	if (answer == NULL)
		n->nlmsg_flags |= NLM_F_ACK;

	status = sendmsg(rtnl->fd, &msg, 0);

	if (status < 0) {
		debug_msg("cannot talk to rtnetlink: %s", strerror(errno));
		return -1;
	}

	memset(buf, 0, sizeof(buf));

	iov.iov_base = buf;

	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(rtnl->fd, &msg, 0);

		if (status < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			debug_msg("netlink received error %s", strerror(errno));
			return -1;
		}

		if (status == 0) {
			debug_msg("EOF on netlink");
			return -1;
		}

		if (msg.msg_namelen != sizeof(nladdr)) {
			debug_msg("sender address length == %d", msg.msg_namelen);
			return -1;
		}

		h = (struct nlmsghdr *)buf;
		while (status >= (int)sizeof(*h)) {
			len = h->nlmsg_len;
			rem_len = len - sizeof(*h);

			if ((rem_len < 0) || (len > status)) {
				if (msg.msg_flags & MSG_TRUNC) {
					debug_msg("truncated message\n");
					return -1;
				}
				debug_msg("malformed message: len=%d\n", len);
				return -1;
			}

			if (((int)nladdr.nl_pid != peer) ||
			    (h->nlmsg_pid != rtnl->local.nl_pid) ||
			    (h->nlmsg_seq != seq)) {
				/* Don't forget to skip that message. */
				status -= NLMSG_ALIGN(len);
				h = (struct nlmsghdr*)((char*)h + NLMSG_ALIGN(len));
				continue;
			}

			if (h->nlmsg_type == NLMSG_ERROR) {
				err = (struct nlmsgerr*)NLMSG_DATA(h);
				if (rem_len < (int)sizeof(struct nlmsgerr)) {
					debug_msg("ERROR truncated");
				} else {
					if (!err->error) {
						if (answer)
							memcpy(answer, h, h->nlmsg_len);
						return 0;
					}

					debug_msg("RTNETLINK answers: %s",
						  strerror(-err->error));
					errno = -err->error;
				}
				return -1;
			}
			if (answer) {
				memcpy(answer, h, h->nlmsg_len);
				return 0;
			}

			debug_msg("unexpected reply");

			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr *)((char*)h + NLMSG_ALIGN(len));
		}
		if (msg.msg_flags & MSG_TRUNC) {
			debug_msg("message truncated");
			continue;
		}
		if (status) {
			debug_msg("%d not parsed bytes", status);
			return -1;
		}
	}

	return 0;
}

static int ev_rtnl_route_cb(struct ev_rtnl_state *rtnl,
			      const struct sockaddr_nl NG_UNUSED(*addr),
			      struct nlmsghdr *n, void *arg)
{
	struct ev_rtnl_router_cb_data *data = arg;
	struct rtmsg *r = NLMSG_DATA(n);
	struct rtattr *tb[RTA_MAX + 1];
	unsigned int ifindex;

	if (r->rtm_family != AF_INET) {
		debug_msg("skipping non IPv4 address (family=%d)",
			  r->rtm_family);
		return 0;
	}

	if (ev_rtnl_parse_attr(tb, RTA_MAX, RTM_RTA(r),
			       n->nlmsg_len - NLMSG_LENGTH(sizeof(*r))) < 0) {
		debug_msg("cannot parse reply");
		return -1;
	}

	/* if an ifindex was specified, make sure that this route is attached to
	 * that specific interface
	 */
	if (data->ifindex) {
		if (!tb[RTA_OIF])
			return 0;

		ifindex = *(unsigned int *)RTA_DATA(tb[RTA_OIF]);
		if (ifindex != data->ifindex)
			return 0;
	}

	return data->cb(rtnl, r, tb, data->ifindex, data->arg);
}

int ev_rtnl_iface_route_for_each(const char *iface, route_cb_t cb, void *arg)
{
	struct ev_rtnl_router_cb_data data;
	struct ev_rtnl_link_req req;
	struct ev_rtnl_state rtnl;
	int ret = -1;

	debug_msg("inspecting routes on %s", iface ? iface : "ALL");

	data.ifindex = 0;
	data.cb = cb;
	data.arg = arg;

	if (iface) {
		data.ifindex = if_nametoindex(iface);
		if (data.ifindex == 0) {
			debug_msg("cannot get ifindex: %s", strerror(errno));
			goto err;
		}
	}

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_GETROUTE;

	req.i.ifi_family = AF_INET;

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		return ret;
	}

	ret = send(rtnl.fd, &req, sizeof(req), 0);
	if (ret < 0)
		goto err;

	ret = ev_rtnl_recv(&rtnl, ev_rtnl_route_cb, &data);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

bool ev_rtnl_iface_link_get(const char *iface)
{
	struct ev_rtnl_state rtnl;
	struct ev_rtnl_link_req req;
	struct ifinfomsg *ifi;
	bool is_up = false;
	int idx;
	struct {
		struct nlmsghdr n;
		char buf[16384];
	} answer;

	debug_msg("iface %s", iface);

	idx = if_nametoindex(iface);
	if (idx == 0) {
		debug_msg("cannot get ifindex: %s", strerror(errno));
		goto err;
	}

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_GETLINK;

	req.i.ifi_family = AF_PACKET;
	req.i.ifi_index = idx;

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	if (ev_rtnl_send(&rtnl, &req.n, 0, 0, &answer.n) < 0)
		goto err;

	ifi = NLMSG_DATA(&answer.n);
	if ((ifi->ifi_flags & IFF_UP) &&
	    (ifi->ifi_flags & IFF_RUNNING)) {
		is_up = true;
		debug_msg("interface %s is UP and RUNNING", iface);
	}
err:
	ev_rtnl_close(&rtnl);
	return is_up;
}

int ev_rtnl_iface_link_set(const char *iface, bool up)
{
	struct ev_rtnl_state rtnl;
	struct ev_rtnl_link_req req;
	int idx, ret = -1;

	debug_msg("set iface %s %s", iface, up ? "up" : "down");

	idx = if_nametoindex(iface);
	if (idx == 0) {
		debug_msg("cannot get ifindex: %s", strerror(errno));
		goto err;
	}

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_NEWLINK;

	req.i.ifi_family = AF_PACKET;
	req.i.ifi_index = idx;
	req.i.ifi_change |= IFF_UP;
	if (up)
		req.i.ifi_flags |= IFF_UP;
	else
		req.i.ifi_flags &= ~IFF_UP;

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	ret = ev_rtnl_send(&rtnl, &req.n, 0, 0, NULL);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

int ev_rtnl_iface_mtu_set(const char *iface, uint32_t mtu)
{
	struct ev_rtnl_state rtnl;
	struct ev_rtnl_link_req req;
	int idx, ret = -1;

	debug_msg("set mtu for %s to %d", iface, mtu);

	idx = if_nametoindex(iface);
	if (idx == 0) {
		debug_msg("cannot get ifindex: %s", strerror(errno));
		goto err;
	}

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_NEWLINK;

	req.i.ifi_family = AF_PACKET;
	req.i.ifi_index = idx;

	NG_ADDATTR(&req.n, sizeof(req), IFLA_MTU, &mtu, 4);

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	ret = ev_rtnl_send(&rtnl, &req.n, 0, 0, NULL);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}


static int ev_rtnl_ip_set(struct ev_rtnl_state *rtnl, int cmd, uint32_t flags,
			  int ifindex, uint32_t ip, uint32_t prefixlen)
{
	struct ev_rtnl_addr_req req;
	int ret = -1;
	uint32_t i;

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_type = cmd;
	req.n.nlmsg_flags = NLM_F_REQUEST | flags;

	req.i.ifa_index = ifindex;
	req.i.ifa_family = AF_INET;
	req.i.ifa_prefixlen = prefixlen;

	NG_ADDATTR(&req.n, sizeof(req), IFA_ADDRESS, &ip, 4);
	NG_ADDATTR(&req.n, sizeof(req), IFA_LOCAL, &ip, 4);

	/* create the broadcast address */
	for (i = 0; i < 32 - prefixlen; i++)
		ip |= 1 << i;

	debug_msg("broadcast address: %d.%d.%d.%d",
		  (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16,
		  (ip & 0x0000ff00) >> 8, ip & 0x000000ff);

	NG_ADDATTR(&req.n, sizeof(req), IFA_BROADCAST, &ip, 4);

	ret = ev_rtnl_send(rtnl, &req.n, 0, 0, NULL);
	if ((ret < 0) && (errno == EEXIST))
		ret = 0;
err:
	return ret;
}

static int ev_rtnl_ip_del(struct ev_rtnl_state NG_UNUSED(*rtnl),
			  const struct sockaddr_nl NG_UNUSED(*addr),
			  struct nlmsghdr *n, void *arg)
{
	struct ifaddrmsg *ifa = NLMSG_DATA(n);
	struct rtattr *tb[IFA_MAX + 1];
	unsigned int *ifindex = arg;
	int ret = -1;
	uint32_t ip;

	/* delete only addresses belonging to the interface we have selected.
	 * If no index has been passed, then flush ALL the IPs
	 */
	if (*ifindex && (ifa->ifa_index != *ifindex))
		return 0;

	if (ifa->ifa_family != AF_INET) {
		debug_msg("skipping non IPv4 address (family=%d)",
			  ifa->ifa_family);
		return 0;
	}

	if (ev_rtnl_parse_attr(tb, IFA_MAX, IFA_RTA(ifa),
			       n->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa))) < 0) {
		debug_msg("cannot parse reply");
		goto err;
	}

	memcpy(&ip, RTA_DATA(tb[IFA_LOCAL]), 4);

	debug_msg("deleting %d.%d.%d.%d/%d - ifindex = %d",
		  (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16,
		  (ip & 0x0000ff00) >> 8, ip & 0x000000ff, ifa->ifa_prefixlen,
		  ifa->ifa_index);

	ret = ev_rtnl_ip_set(rtnl, RTM_DELADDR, 0, ifa->ifa_index, ip,
			     ifa->ifa_prefixlen);
err:
	return ret;
}

int ev_rtnl_iface_ip_add(const char *iface, uint32_t ip, uint32_t prefixlen)
{
	struct ev_rtnl_state rtnl;
	int ifindex, ret = -1;

	debug_msg("adding %d.%d.%d.%d/%d to iface %s",
		  (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16,
		  (ip & 0x0000ff00) >> 8, ip & 0x000000ff, prefixlen, iface);

	ifindex = if_nametoindex(iface);
	if (ifindex == 0) {
		debug_msg("cannot get ifindex: %s", strerror(errno));
		goto err;
	}

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	ret = ev_rtnl_ip_set(&rtnl, RTM_NEWADDR, NLM_F_CREATE | NLM_F_REPLACE,
			     ifindex, ip, prefixlen);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

int ev_rtnl_iface_ip_flush(const char *iface)
{
	struct ev_rtnl_state rtnl;
	struct ev_rtnl_link_req req;
	int ret = -1, ifindex = 0;

	debug_msg("flushing iface %s", iface ? iface : "ALL");

	if (iface) {
		ifindex = if_nametoindex(iface);
		if (ifindex == 0) {
			debug_msg("cannot get ifindex: %s", strerror(errno));
			goto err;
		}
	}

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_GETADDR;

	req.i.ifi_family = AF_INET;

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	ret = send(rtnl.fd, &req, sizeof(req), 0);
	if (ret < 0)
		goto err;

	ret = ev_rtnl_recv(&rtnl, ev_rtnl_ip_del, &ifindex);
	if (ret < 0)
		goto err;
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

int ev_rtnl_bridge_create(const char *bridge)
{
	int ret = -1, len = strlen(bridge);
	struct ev_rtnl_link_req req;
	struct ev_rtnl_state rtnl;
	struct rtattr *linkinfo;

	debug_msg("creating bridge %s", bridge);

	if ((len <= 1) || (len >= IFNAMSIZ)) {
		debug_msg("bridge name has invalid length");
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
	req.n.nlmsg_type = RTM_NEWLINK;
	req.i.ifi_family = AF_PACKET;

	linkinfo = NLMSG_TAIL(&req.n);
	NG_ADDATTR(&req.n, sizeof(req), IFLA_LINKINFO, NULL, 0);
	NG_ADDATTR(&req.n, sizeof(req), IFLA_INFO_KIND, "bridge", 6);
	linkinfo->rta_len = (uint8_t *)NLMSG_TAIL(&req.n) - (uint8_t *)linkinfo;

	NG_ADDATTR(&req.n, sizeof(req), IFLA_IFNAME, bridge, len);

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	ret = ev_rtnl_send(&rtnl, &req.n, 0, 0, NULL);
	if ((ret < 0) && (errno == EEXIST))
		ret = 0;
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

int ev_rtnl_iface_destroy(const char *iface)
{
	int ret = -1, len = strlen(iface);
	struct ev_rtnl_link_req req;
	struct ev_rtnl_state rtnl;

	debug_msg("destroying iface %s", iface);

	if ((len <= 1) || (len >= IFNAMSIZ)) {
		debug_msg("iface name has invalid length");
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_DELLINK;

	req.i.ifi_family = AF_PACKET;
	req.i.ifi_index = if_nametoindex(iface);
	if (req.i.ifi_index == 0) {
		debug_msg("cannot find iface %s", iface);
		return -1;
	}

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	ret = ev_rtnl_send(&rtnl, &req.n, 0, 0, NULL);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

static int ev_rtnl_bridge_set(const char *bridge, const char *slave)
{
	struct ev_rtnl_link_req req;
	struct ev_rtnl_state rtnl;
	int ifindex = 0, ret = -1;

	debug_msg("enslaving %s in bridge %s", slave, bridge);

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_NEWLINK;

	req.i.ifi_family = AF_PACKET;
	req.i.ifi_index = if_nametoindex(slave);
	if (req.i.ifi_index == 0) {
		debug_msg("cannot find slave iface %s", slave);
		return -1;
	}

	if (bridge) {
		ifindex = if_nametoindex(bridge);
		if (ifindex == 0) {
			debug_msg("cannot find bridge iface %s", bridge);
			return -1;
		}
	}
	NG_ADDATTR(&req.n, sizeof(req), IFLA_MASTER, &ifindex, 4);

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	ret = ev_rtnl_send(&rtnl, &req.n, 0, 0, NULL);
	if ((ret < 0) && (errno == EEXIST))
		ret = 0;
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

int ev_rtnl_bridge_addif(const char *bridge, const char *slave)
{
	return ev_rtnl_bridge_set(bridge, slave);
}

int ev_rtnl_bridge_delif(const char *slave)
{
	return ev_rtnl_bridge_set(NULL, slave);
}

int ev_rtnl_vlan_add(const char *iface, uint16_t vid)
{
	struct rtattr *linkinfo, *vlaninfo;
	struct ev_rtnl_link_req req;
	struct ev_rtnl_state rtnl;
	char vlan_iface[IFNAMSIZ];
	int ifindex, ret = -1;

	debug_msg("creating vlan %s.%d", iface, vid);

	if (vid > 4094) {
		debug_msg("invalid vid: %d", vid);
		return -1;
	}

	ifindex = if_nametoindex(iface);
	if (ifindex == 0) {
		debug_msg("cannot find iface %s", iface);
		return -1;
	}

	snprintf(vlan_iface, sizeof(vlan_iface), "%s.%d", iface, vid);
	vlan_iface[sizeof(vlan_iface) - 1] = '\0';

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.i));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
	req.n.nlmsg_type = RTM_NEWLINK;
	req.i.ifi_family = AF_PACKET;

	linkinfo = NLMSG_TAIL(&req.n);
	NG_ADDATTR(&req.n, sizeof(req), IFLA_LINKINFO, NULL, 0);
	NG_ADDATTR(&req.n, sizeof(req), IFLA_INFO_KIND, "vlan", 4);
	vlaninfo = NLMSG_TAIL(&req.n);
	NG_ADDATTR(&req.n, sizeof(req), IFLA_INFO_DATA, NULL, 0);
	NG_ADDATTR(&req.n, sizeof(req), IFLA_VLAN_ID, &vid, 2);
	vlaninfo->rta_len = (uint8_t *)NLMSG_TAIL(&req.n) - (uint8_t *)vlaninfo;
	linkinfo->rta_len = (uint8_t *)NLMSG_TAIL(&req.n) - (uint8_t *)linkinfo;

	NG_ADDATTR(&req.n, sizeof(req), IFLA_LINK, &ifindex, 4);
	NG_ADDATTR(&req.n, sizeof(req), IFLA_IFNAME, vlan_iface,
		   strlen(vlan_iface));

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	ret = ev_rtnl_send(&rtnl, &req.n, 0, 0, NULL);
	if ((ret < 0) && (errno == EEXIST))
		ret = 0;
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

static int ev_rtnl_route_set(struct ev_rtnl_state *rtnl, int cmd,
			     uint32_t flags, int ifindex, uint32_t dst,
			     uint32_t prefixlen, uint32_t gw,
			     enum rt_class_t table, enum rt_scope_t scope,
			     int type)
{
	struct ev_rtnl_route_req req;
	int ret = -1;

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.r));
	req.n.nlmsg_type = cmd;
	req.n.nlmsg_flags = NLM_F_REQUEST | flags;

	req.r.rtm_family = AF_INET;
	req.r.rtm_scope = scope;
	req.r.rtm_type = type;
	req.r.rtm_dst_len = prefixlen;

	if (table < 256) {
		req.r.rtm_table = table;
	} else {
		req.r.rtm_table = RT_TABLE_UNSPEC;
		NG_ADDATTR(&req.n, sizeof(req), RTA_TABLE, &table, 4);
	}

	if (dst > 0) {
		/* erase the non masked part of the destination address */
		dst >>= 32 - prefixlen;
		dst <<= 32 - prefixlen;
		NG_ADDATTR(&req.n, sizeof(req), RTA_DST, &dst, 4);
	}
	if (gw > 0)
		NG_ADDATTR(&req.n, sizeof(req), RTA_GATEWAY, &gw, 4);
	if (ifindex > 0)
		NG_ADDATTR(&req.n, sizeof(req), RTA_OIF, &ifindex, 4);

	ret = ev_rtnl_send(rtnl, &req.n, 0, 0, NULL);
	if ((ret < 0) && (errno == EEXIST))
		ret = 0;
err:
	return ret;
}

int ev_rtnl_iface_route_add(const char *iface, uint32_t dst, uint32_t prefixlen,
			    uint32_t gw, uint32_t table)
{
	struct ev_rtnl_state rtnl;
	int ifindex = 0, ret = -1;

	debug_msg("adding %d.%d.%d.%d/%d via %d.%d.%d.%d dev %s table %d",
		  (dst & 0xff000000) >> 24, (dst & 0x00ff0000) >> 16,
		  (dst & 0x0000ff00) >> 8, dst & 0x000000ff, prefixlen,
		  (gw & 0xff000000) >> 24, (gw & 0x00ff0000) >> 16,
		  (gw & 0x0000ff00) >> 8, gw & 0x000000ff, iface, table);

	if (iface) {
		ifindex = if_nametoindex(iface);
		if (ifindex == 0) {
			debug_msg("cannot get ifindex: %s", strerror(errno));
			goto err;
		}
	}

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	if (table == 0)
		table = RT_TABLE_MAIN;

	ret = ev_rtnl_route_set(&rtnl, RTM_NEWROUTE, NLM_F_CREATE | NLM_F_REPLACE,
				ifindex, dst, prefixlen, gw, table,
				RT_SCOPE_UNIVERSE, RTN_UNICAST);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

int ev_rtnl_iface_route_del(const char *iface, uint32_t dst, uint32_t prefixlen,
			    uint32_t gw, uint32_t table)
{
	struct ev_rtnl_state rtnl;
	int ifindex, ret = -1;

	debug_msg("deleting %d.%d.%d.%d/%d via %d.%d.%d.%d dev %s table %d",
		  (dst & 0xff000000) >> 24, (dst & 0x00ff0000) >> 16,
		  (dst & 0x0000ff00) >> 8, dst & 0x000000ff, prefixlen,
		  (gw & 0xff000000) >> 24, (gw & 0x00ff0000) >> 16,
		  (gw & 0x0000ff00) >> 8, gw & 0x000000ff, iface, table);

	ifindex = if_nametoindex(iface);
	if (ifindex == 0) {
		debug_msg("cannot get ifindex: %s", strerror(errno));
		goto err;
	}

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	if (table == 0)
		table = RT_TABLE_MAIN;

	ret = ev_rtnl_route_set(&rtnl, RTM_DELROUTE, 0, ifindex, dst, prefixlen,
				gw, 0, RT_SCOPE_NOWHERE, 0);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

static int ev_rtnl_route_flush_one(struct ev_rtnl_state NG_UNUSED(*rtnl),
				   struct rtmsg *r, struct rtattr **tb,
				   unsigned int ifindex, void NG_UNUSED(*arg))
{
	uint32_t dst = 0, gw = 0;

	if (tb[RTA_DST])
		dst = *(uint32_t *)RTA_DATA(tb[RTA_DST]);

	if (tb[RTA_GATEWAY])
		gw = *(uint32_t *)RTA_DATA(tb[RTA_GATEWAY]);

	debug_msg("flushing %d.%d.%d.%d/%d via %d.%d.%d.%d ifindex %d table %d scope %d type %d",
		  (dst & 0xff000000) >> 24, (dst & 0x00ff0000) >> 16,
		  (dst & 0x0000ff00) >> 8, dst & 0x000000ff, r->rtm_dst_len,
		  (gw & 0xff000000) >> 24, (gw & 0x00ff0000) >> 16,
		  (gw & 0x0000ff00) >> 8, gw & 0x000000ff, ifindex,
		  r->rtm_table, r->rtm_scope, r->rtm_type);

	return ev_rtnl_route_set(rtnl, RTM_DELROUTE, 0, ifindex, dst,
				 r->rtm_dst_len, gw, r->rtm_table,
				 RT_SCOPE_NOWHERE, 0);
}

int ev_rtnl_iface_route_flush(const char *iface)
{
	debug_msg("flushing iface %s", iface ? iface : "ALL");

	return ev_rtnl_iface_route_for_each(iface,
					    ev_rtnl_route_flush_one,
					    NULL);
}

static int ev_rtnl_rule_set(struct ev_rtnl_state *rtnl, int cmd,
			    uint32_t flags, const char *iifname, uint32_t src,
			    uint32_t dst, uint32_t prefixlen,
			    enum rt_class_t table, enum rt_scope_t scope,
			    int type)
{
	struct ev_rtnl_route_req req;
	int ret = -1;

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(req.r));
	req.n.nlmsg_type = cmd;
	req.n.nlmsg_flags = NLM_F_REQUEST | flags;

	req.r.rtm_family = AF_INET;
	req.r.rtm_scope = scope;
	req.r.rtm_type = type;
	req.r.rtm_dst_len = prefixlen;

	if (table < 256) {
		req.r.rtm_table = table;
	} else {
		req.r.rtm_table = RT_TABLE_UNSPEC;
		NG_ADDATTR(&req.n, sizeof(req), FRA_TABLE, &table, 4);
	}

	if (src > 0) {
		req.r.rtm_src_len = 32;
		NG_ADDATTR(&req.n, sizeof(req), FRA_SRC, &src, 4);
	}

	if (dst > 0) {
		/* erase the non masked part of the destination address */
		dst >>= 32 - prefixlen;
		dst <<= 32 - prefixlen;
		NG_ADDATTR(&req.n, sizeof(req), FRA_DST, &dst, 4);
	}

	if (iifname)
		NG_ADDATTR(&req.n, sizeof(req), FRA_IFNAME, iifname,
			   strlen(iifname) + 1);

	ret = ev_rtnl_send(rtnl, &req.n, 0, 0, NULL);
	if ((ret < 0) && (errno == EEXIST))
		ret = 0;
err:
	return ret;
}

int ev_rtnl_iface_rule_add(const char *iiface, uint32_t src, uint32_t dst,
			   uint32_t prefixlen, uint32_t table)
{
	struct ev_rtnl_state rtnl;
	int ret = -1;

	debug_msg("adding iif %s from %d.%d.%d.%d to %d.%d.%d.%d/%d table %d",
		  iiface, (src & 0xff000000) >> 24, (src & 0x00ff0000) >> 16,
		  (src & 0x0000ff00) >> 8, src & 0x000000ff,
		  (dst & 0xff000000) >> 24, (dst & 0x00ff0000) >> 16,
		  (dst & 0x0000ff00) >> 8, dst & 0x000000ff, prefixlen,
		  table);

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	if (table == 0)
		table = RT_TABLE_MAIN;

	ret = ev_rtnl_rule_set(&rtnl, RTM_NEWRULE, NLM_F_CREATE | NLM_F_EXCL,
			       iiface, src, dst, prefixlen, table,
			       RT_SCOPE_UNIVERSE, RTN_UNICAST);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

int ev_rtnl_iface_rule_del(const char *iiface, uint32_t src, uint32_t dst,
			   uint32_t prefixlen, uint32_t table)
{
	struct ev_rtnl_state rtnl;
	int ret = -1;

	debug_msg("deleting iif %s from %d.%d.%d.%d to %d.%d.%d.%d/%d table %d",
		  iiface, (src & 0xff000000) >> 24, (src & 0x00ff0000) >> 16,
		  (src & 0x0000ff00) >> 8, src & 0x000000ff,
		  (dst & 0xff000000) >> 24, (dst & 0x00ff0000) >> 16,
		  (dst & 0x0000ff00) >> 8, dst & 0x000000ff, prefixlen,
		  table);

	if (ev_rtnl_open(&rtnl, 0) < 0) {
		debug_msg("cannot open rtnl link");
		goto err;
	}

	if (table == 0)
		table = RT_TABLE_MAIN;

	ret = ev_rtnl_rule_set(&rtnl, RTM_DELRULE, 0, iiface, src, dst,
			       prefixlen, table, RT_SCOPE_UNIVERSE, RTN_UNSPEC);
err:
	ev_rtnl_close(&rtnl);
	return ret;
}

static int ev_rtnl_route_is_default(struct ev_rtnl_state NG_UNUSED(*rtnl),
				    struct rtmsg NG_UNUSED(*r),
				    struct rtattr **tb,
				    unsigned int NG_UNUSED(ifindex), void *arg)
{
	uint32_t dst = 0, gw = 0;
	bool *is_default = arg;

	if (tb[RTA_DST])
		dst = *(uint32_t *)RTA_DATA(tb[RTA_DST]);

	if (tb[RTA_GATEWAY])
		gw = *(uint32_t *)RTA_DATA(tb[RTA_GATEWAY]);

	/* check if this is a default route */
	if ((gw > 0) && (dst == 0))
		*is_default = true;

	return 0;
}

bool ev_rtnl_route_has_default(const char *iface)
{
	bool has_default = false;
	int ret;

	debug_msg("searching for default route on %s", iface ? iface : "ALL");

	ret = ev_rtnl_iface_route_for_each(iface,
					   ev_rtnl_route_is_default,
					   &has_default);
	if (ret < 0)
		debug_msg("error while searching for default route");

	debug_msg("iface=%s has_default=%d", iface, has_default);

	return has_default;
}
