
#ifndef  __NET_INFO__H
#define  __NET_INFO__H
#include <stdio.h>      
#include <sys/types.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <net/if.h>
#include "serv_config.h"
//#define   JOIN_SERVER1_IP	"192.168.0.253"//"124.202.140.203"
//#define   JOIN_SERVER2_IP	"192.168.0.2"
//#define   JOIN_SERVER_PORT	  8500//8531//8500
#define   SOCK_TIME_OUT		 8

// 获取IP地址程序
int Get_ip(unsigned char * ipaddr);
int Get_ip_str(unsigned char * ipaddr);

// 获取MAC地址程序
int Get_mac(unsigned  char *mac_addr);


int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen);
int sock_close(int sockfd);

#endif   // net_info.h

