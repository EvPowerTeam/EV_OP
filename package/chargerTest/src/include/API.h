
#ifndef __API__H_
#define __API__H_

/* system call  and C library head file */

#include <sys/types.h>		/* basic system data types */
#include <sys/socket.h>		/* basic socket definitions */
#include <sys/time.h>		/* timeval{} for select */
#include <time.h>		/* timespec{} for pselect */
#include <netinet/in.h>		/* sockaddr_in{} and other Inrernet defines */
#include <arpa/inet.h>		/* iner(3) functions */
#include <errno.h>
#include <fcntl.h>		/* for nonblocking */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/resource.h>
#include <syslog.h>
#include <unistd.h>
#include <mqueue.h>


#endif 

