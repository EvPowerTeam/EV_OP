
#ifndef __API__H_
#define __API__H_

/* system call  and C library head file */

#include <sys/types.h>		/* basic system data types */
#include <sys/socket.h>		/* basic socket definitions */
#include <sys/time.h>		/* timeval{} for select */
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>		/* sockaddr_in{} and other Inrernet defines */
#include <arpa/inet.h>		/* iner(3) functions */
#include <net/if.h>
#include <ifaddrs.h>

#include <errno.h>
#include <time.h>		/* timespec{} for pselect */
#include <fcntl.h>		/* for nonblocking */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <mqueue.h>


#endif 

