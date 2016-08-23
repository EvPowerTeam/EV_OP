#include <libev/msgqueue.h>
#include "libev.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <mqueue.h>

struct mq_attr attr;
#define FILEMODE (S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP)

/*
 * Save name of queue by yourself!
 * return 0 if sucess
 * return -1 if unsucess
 * */
int mqcreate(int flag, int maxmsg, long int msgsize, const char *name)
{
	int flags;
	mqd_t mqd;

	if (maxmsg < 0 || maxmsg > 10 || msgsize > 8192 || msgsize < 0) {
		debug_msg("params exceed");
		return -1;
	}

	flags = O_RDWR | O_CREAT | flag;
	//flags |= O_EXCL;

	attr.mq_maxmsg = maxmsg;
	attr.mq_msgsize = msgsize;
    
	if ((attr.mq_maxmsg != 0 && attr.mq_msgsize == 0) || (attr.mq_maxmsg == 0 && attr.mq_msgsize !=0)){
		debug_msg("must specify both maxmsg and msgsize");
		return -1;
	}

	mqd = mq_open(name, flags, FILEMODE, (attr.mq_maxmsg != 0) ? &attr : NULL);
	debug_msg("%d  %d", mqd, errno);
	mq_close(mqd);
	return 0;
}

int mqsend(const char *name, char *msg, long int msglen, unsigned int prio)
{
	mqd_t mqd;
	struct mq_attr attr;

	if (strlen(msg) != msglen || !name || !msg) {
		debug_msg("params wrong");
		return -1;
	}

	if ((mqd = mq_open(name, O_WRONLY)) == -1){
		debug_msg("open error: %d", errno);
		return -1;
	}
	mq_getattr(mqd, &attr);

	return mq_send(mqd, msg, msglen, prio);		//print errno?
}


char *mqreceive(const char *name)
{
	int flags;
	mqd_t mqd;
	ssize_t n;
	unsigned int prio;
	char *msg;
	struct mq_attr attr;
	flags = O_RDONLY | O_NONBLOCK;
	//flags = O_RDONLY;
	mqd = mq_open(name, flags);
	mq_getattr(mqd, &attr);

	msg = malloc(attr.mq_msgsize);

	n = mq_receive(mqd, msg, attr.mq_msgsize, &prio);
	debug_msg("read %ld bytes, priority = %u buffer = %s\n",(long) n, prio, msg);
	return msg;
}

char *mqreceive_timed(const char *name, int lenth)
{
	int flags;
	mqd_t mqd;
	ssize_t n;
	unsigned int prio;
	char *msg, *result;
	struct mq_attr attr;
	struct timespec abs_timeout;
	flags = O_RDONLY;
	clock_gettime(CLOCK_REALTIME, &abs_timeout);
	abs_timeout.tv_sec+=4;

	mqd = mq_open(name, flags);
	mq_getattr(mqd, &attr);

	if (lenth > attr.mq_msgsize)
		return NULL;

	msg = malloc(attr.mq_msgsize);
	if ( (mq_timedreceive(mqd, msg, attr.mq_msgsize, &prio, &abs_timeout))  < 0) {
		free(msg);
		return NULL;
	}
	debug_msg("read %ld bytes, priority = %u buffer = %s\n",(long) n, prio, msg);
	result = strndup(msg, lenth);
	free(msg);
	return result;
}

