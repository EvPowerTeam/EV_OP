


#include "./include/API.h"
#include "./include/system_call.h"
#include <mqueue.h>

int 
Socket(int domain, int type, int protocol)
{
	int n;

	if ( (n = socket(domain, type, protocol)) < 0)
		err_sys("socket error");
	return (n);
}

int
Bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen)
{
	int n;

	if ( (n = bind(sockfd, addr, addrlen)) < 0)
		err_sys("bind error");
	return (n);
}

int 
Listen(int sockfd, int backlog)
{
	int n;

	if ( (n = listen(sockfd, backlog)) < 0)
		err_sys("listen error");
	return (n);
}

int 
Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int n;

	if ( (n = accept(sockfd, addr, addrlen)) < 0)
		err_sys("accept error");
	return (n);
}

int 
Access(const char *pathname, int mode)
{
	int n;

	if ( (n = access(pathname, mode)) < 0)
		err_sys("access error");
	return (n);	
}

int Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	int n;

	if(setsockopt(sockfd, level, optname, optval, optlen) < 0)
		err_sys("setsockopt error");
	return (n);
}

#if 0
mqd_t   Mq_open(const char *path, int oflag)
{
    mqd_t   mqid;

    if ( (mqid = mq_open(path, oflag)) < 0)
        err_sys("mq_open failed");
   
    return mqid;
}
#endif

