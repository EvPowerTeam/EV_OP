#include "mq.h"
#include "include/dashboard.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <mqueue.h>


#define FILEMODE (S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP)
struct mq_attr attr;

int mqcreate_debug(int argc, char **argv, char DASH_UNUSED(*extra_arg))
{
    int c, flags;
    mqd_t mqd;

    flags = O_RDWR | O_CREAT;
    while((c = getopt(argc, argv, "em:z:")) != -1) {//处理参数，带冒号的表示后面有参数的
        switch(c) {
            case 'e':
                flags |= O_EXCL;
                printf("the optind is :%d\n",optind);
                break;
            case 'm':
                attr.mq_maxmsg = atol(optarg);
                printf("the optind is :%d\n",optind);
                break;
            case 'z':
                attr.mq_msgsize = atol(optarg);
                printf("the optind is :%d\n",optind);
                break;
        }
    }
    if (optind != argc - 1) {
        debug_msg("usage: mqcreate [-e] [-m maxmsg -z msgsize] <name>");
        exit(0);
    }
    
    if ((attr.mq_maxmsg != 0 && attr.mq_msgsize == 0) || (attr.mq_maxmsg == 0 && attr.mq_msgsize !=0)){
        debug_msg("must specify both -m maxmsg and -z msgsize");
        exit(0);
    }

    mqd = mq_open(argv[optind], flags, FILEMODE, (attr.mq_maxmsg != 0) ? &attr : NULL);
    debug_msg("%d  %d", mqd, errno);
    mq_close(mqd);
    return 0;
}

int mqreceive_debug(int argc, char **argv, char DASH_UNUSED(*extra_arg))
{
    int c, flags;
    mqd_t mqd;
    ssize_t n;
    unsigned int prio;
    char *buff;
    struct mq_attr attr;
    struct timespec abs_timeout;
    flags = O_RDONLY;
        clock_gettime(CLOCK_REALTIME, &abs_timeout);
        abs_timeout.tv_sec+=4;
    while((c = getopt(argc, argv, "n")) != -1){
        switch(c) {
            case 'n':
                //flags |= O_NONBLOCK;
                break;
        }
    }
    if (optind != argc-1) {
        debug_msg("usage: mqreceive [-n] <name>");
        exit(0);
    }

    mqd = mq_open(argv[optind], flags);
    mq_getattr(mqd, &attr);

    buff = malloc(attr.mq_msgsize);

    //n = mq_receive(mqd, buff, attr.mq_msgsize, &prio);
    debug_msg("%ld", abs_timeout.tv_sec);
    n = (mq_timedreceive(mqd, buff, attr.mq_msgsize, &prio, &abs_timeout));
    mq_close(mqd);
    debug_msg("read %ld bytes, priority = %u buffer = %s\n",(long) n, prio, buff);
    free(buff);
	return 0;
}

int mqsend_debug(int argc, char **argv, char DASH_UNUSED(*extra_arg))
{
    mqd_t mqd;
    const char *ptr = "message 1";
    size_t len;
    unsigned int prio;

    if (argc != 4) {
        printf("usage: mqsend <name> <#bytes> <priority>");
        exit(0);
	}
	len = atoi(argv[2]);
	prio = atoi(argv[3]);

	if ((mqd = mq_open(argv[1], O_WRONLY)) == -1){
		printf("open error");
		exit(0);
	}
	//ptr = calloc(len, sizeof(char));

	mq_send(mqd, ptr, len, prio);
	return 0;
}
