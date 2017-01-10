
#ifndef  __EV_LIB__H
#define  __EV_LIB__H

#include "api_head.h"

// 普通函数

/* get ip addr*/
int get_ip(char * ipaddr);
int get_ip_str(char * ipaddr);
/* get mac addr*/
int get_mac(char *mac_addr);
// reply sock connect
int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen);
int sock_close(int sockfd);
size_t writen(int fd, const void *buf, size_t count);
int readable_timeout(int fd, int sec);
int writeable_timeout(int fd, int sec);

// 包裹函数
int Socket(int , int, int);
int Bind(int , const struct sockaddr *, socklen_t);
int Listen(int, int);
int Accept(int, struct sockaddr *, socklen_t *);
int Access(const char *, int);
int Setsockopt(int, int, int, const void *, socklen_t);



#endif // ev_lib.h

