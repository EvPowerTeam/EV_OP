#include <sys/types.h>
#include <signal.h>
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
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <syslog.h>

#include "data.h"
#include "net_info.h"
#include "protocal_conn.h"
#include "AES.h"

#define  HB_SUCCESS  	0x01
#define  HB_FAILSE	0x02
#define  LOCKFILE	"/var/run/EV_HB.pid"
#define	 LOCKMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

unsigned  char  G_StopCnt =0;
char get_HB(float data);
void GetPresentHB(int savefd, char flag);
void daemonize(const char *cmd);
int already_running(void);
void sig_handler(int signo)
{
	printf("SIGPIPE  \n");

}

int sock_conn_server(struct sockaddr_in *cliaddr, struct connect_serv *conn) 
{
	int serv_fd;
	int ops;
 	serv_fd = sock_server_init(cliaddr, conn->IP, conn->Port);
//	conn->CS_Port++;
//	bind(serv_fd, (struct sockaddr*)&cliaddr, sizeof(struct sockaddr_in));
#if 1
	ops = fcntl(serv_fd, F_GETFL);
	if(ops < 0)
	{
		return -1;
	}
	
	ops = ops | O_NONBLOCK;
	if(fcntl(serv_fd, F_SETFL, ops) < 0)
	{
		return -1;
	} 
#endif
	if( !connect_retry(serv_fd, (const struct sockaddr *)cliaddr, sizeof(struct sockaddr_in)) ){
		close(serv_fd);
		printf("connect server connect fail...\n");
		return -1;
	}
#if 0
	ops = fcntl(serv_fd, F_GETFL);
	if(ops < 0)
	{
		return -1;
	}
	
	ops = ops &(~ O_NONBLOCK);
	if(fcntl(serv_fd, F_SETFL, ops) < 0)
	{
		return -1;
	} 
#endif
	printf("连接connect server 成功...%d\n", serv_fd);
	return serv_fd;
}


int  open_saveHB_file( const char *path)
{
		struct stat st;
		int fd;
		if(stat(path, &st) == -1)
		{
				perror("stat:");
				if( (fd = open(path, O_CREAT| O_RDWR|O_EXCL|O_TRUNC, 0666)) < 0)
				{
						perror("open saveHB_file:");
						exit(1);
				}	
		}
		else
		{
				if((fd = open(path, O_RDWR|O_TRUNC, 0666)) < 0)
				{
						perror("open saveHB_file:");
						exit(1);
				}
		}
		return fd;
}

int open_Rid_file(const char *path)
{
		struct stat st;
		int fd;
		if(stat(path, &st) == -1)
		{
				perror("stat:");
				if( (fd = open(path, O_CREAT| O_RDWR|O_EXCL|O_TRUNC, 0666)) < 0)
				{
						perror("open saveRID_file:");
						exit(1);
				}
				system("echo 1011 > /bin/saveRID");
		}
		else
		{
				if((fd = open(path, O_RDWR, 0666)) < 0)
				{
						perror("open saveRID_file:");
						exit(1);
				}
		}
		return fd;

}

int read_Rid(int fd, unsigned int * cid)
{
		unsigned char buff[20] = {0};
		unsigned char Rid[20] ={0};
		unsigned int CID = 0;
		char Rid_len;
		char i;
		char cnt=0;
		lseek(fd, SEEK_SET, 0);
		Rid_len = read(fd, buff, 20);
		printf("read fid = %s\n", buff);
		for(i=0; i<Rid_len; i++)
		{
			if( 48 <= buff[i] && buff[i] <= 57)
			{
				Rid[cnt] = buff[i];
				cnt++;
			}
		}
		if(cnt == 8)
		{
			CID = atoi(Rid);
			printf("Read CID = %d\n", CID);
			if(CID != *cid)
			{
				*cid = CID;
				printf("CID 不相等.............................................\n");
				return 1;
			}
		}		
	return 0;
}

int   main(int   argc,   char   *argv[]) 
{



	int sock_serv_fd;
	struct sockaddr_in   cliaddr;
	struct connect_serv   conn_serv;
	unsigned int cid = 47001001;
	int savefd, Rid_fd;
	char buff[5] ={0};
	signal(SIGPIPE, sig_handler);
	daemonize(argv[0]);

	if(already_running())
	{
		syslog(LOG_ERR, "%s[%ld] is already running...", argv[0], (long int)getpid());
		exit(1);
	}
	
	savefd = open_saveHB_file("/tmp/saveHB");
	Rid_fd = open_Rid_file("/bin/saveRID");
while(1)
{
	if(!protocol_init(cid, 0)){
		system("echo 0 > /tmp/saveHB");
		printf("协议初始化失败...\n");
//		exit(1);
		sleep(10);
		continue;
	}
	break;
}

while(1)
{
	system("echo 0 > /tmp/saveHB");
	
	if(!protocol_init(cid, 1))
	{
		goto waitTime_err;
	}	
	
	memcpy(IV, KEYA, 16);

	HB_DateTime_cnt = 0;
	sock_serv_fd = sock_server_init(&cliaddr, JOIN_SERVER1_IP, JOIN_SERVER_PORT);	

	if(  !connect_retry(sock_serv_fd, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) ){
		// 连接失败，然后连接join_server2_ip
		printf("jion server1 connect fail...\n");
		close(sock_serv_fd);
		if(!(sock_serv_fd = sock_server_init(&cliaddr, JOIN_SERVER2_IP, JOIN_SERVER_PORT))){
			close(sock_serv_fd);
		  	printf("join server2 connect fail...\n");
			goto waitTime_err;
		}
	}

	printf("客户端连接成功...\n"); 
	char sock_buff[128] = {0};

	// 连接join server
	if( !join_conn_serv(sock_serv_fd, &conn_serv)){
		printf("与通信JOIN server 失败, 重新初始化...\n");
		sleep(2);
		goto  waitTime_err;
	}

//	if(!join_comfirm_serv(sock_serv_fd, &conn_serv, CHARGER_READY, ONLINE)){
//		printf("发送JOIN SERVER确认连接失败...\n");
//		exit(1);
//	}
//	sleep(2);

//	break;


	printf("\n    #############################################################\n");
	printf("     #############################################################\n");
	printf("关闭JOIN SERVER连接，开始连接CONNECT SERVER...\n");	// JOIN SERVER 连接
	close(sock_serv_fd);
	sock_serv_fd = -1;
	memset(&cliaddr, 0, sizeof(cliaddr));
	printf("conn IP:%s port:%d\n",  conn_serv.IP, conn_serv.Port);
//	HB_DateTime_cnt = 0;

int g_CMD = SOCK_CMD_REQ;
int i;
char diff;
int wr = 0;

while(1){
//	printf("sock_conn_fd=%d\n", sock_serv_fd);
	

	switch(g_CMD)
	{
	
	// 发送新连接
	 case  SOCK_CMD_REQ:
		if(G_StopCnt == 10)   // 发送失败之后，次数到20次，执行goto，重新连接join server
			goto	waitTime_err;
		if((sock_serv_fd = sock_conn_server(&cliaddr, &conn_serv))<0)
		{
			G_StopCnt++;	
			continue;
		}
		protocol_CSREQ_ready();
		if( !CServer_send(sock_serv_fd, (u8 *)CServer_req, sizeof(CServer_REQ))){
			sock_close(sock_serv_fd);
			memset(&cliaddr, 0, sizeof(cliaddr));
			printf("发送新连接失败...\n");
			sleep(2);
			G_StopCnt++;
			continue;
		}
		sleep(1);
		sock_close(sock_serv_fd);
		g_CMD = SOCK_CMD_FUP;
		G_StopCnt = 0;
	continue;
	
	case 	SOCK_CMD_FUP:
	// 发送全更新
		if(G_StopCnt == 10)
			goto	waitTime_err;
		if((sock_serv_fd = sock_conn_server(&cliaddr, &conn_serv))<0)
		{
			G_StopCnt++;	
			continue;
		}
		protocol_CSFUP_ready();
		if( !CServer_send(sock_serv_fd, (u8 *)CS_FUP, sizeof(CServer_FULL_UPDATE))){
			sock_close(sock_serv_fd);
			memset(&cliaddr, 0, sizeof(cliaddr));
			printf("发送全更新失败...\n");
			sleep(10);
			G_StopCnt++;
			continue;
		}
		sleep(1);
		sock_close(sock_serv_fd);
		G_StopCnt = 0;
		g_CMD = SOCK_CMD_HB;
	continue;
	
	case SOCK_CMD_HB:

	// 发送心跳
		printf("未连接次数：%d\n", G_StopCnt);
		if(read_Rid(Rid_fd, &cid))
		{
			goto waitTime_err;
		}
		if(G_StopCnt == 60)
		{
			GetPresentHB(savefd, HB_FAILSE);
			goto	waitTime_err;
		}
		if((sock_serv_fd = sock_conn_server(&cliaddr, &conn_serv))<0)
		{
			G_StopCnt++;	
			break;
		}
		protocol_CSHB_ready();
		if( !CServer_send(sock_serv_fd, (u8 *)&CS_HB, sizeof(CS_HB))){
			printf("发送心跳失败...\n");
			sock_close(sock_serv_fd);
			memset(&cliaddr, 0, sizeof(cliaddr));
			G_StopCnt++;
//			sleep(2);
			break;
		}
		G_StopCnt = 0;
		sleep(1);
		sock_close(sock_serv_fd);

		GetPresentHB(savefd, HB_SUCCESS);

 		continue;
	 } // switch
	GetPresentHB(savefd, HB_FAILSE); // 接收心跳失败，计算心跳率
	

      } //  while(1) of conn server

waitTime_err:
		sleep(5);
		printf("长时间连接connect timeout，返回join server reconnect...\n");
		if(read_Rid(Rid_fd, &cid))
		{
			goto waitTime_err;
		}
//		free(CServer_req);
//		free(ConnectCF);
//		free(CS_FUP);
//		free(CS_PUP);
		G_StopCnt = 0;
  } // while(1) of join server		
}

char get_HB(float data)
{
	int i;
	int tmp = 10;
	char hb = 0;
	for(i = 0; i<6; i++)
	{
		if( (int)( data*tmp ) > 0)
		{
			hb = (int)(data *tmp*10);
			printf("HB=%d\n", hb);
			break;	
		}
		tmp *= 10;
//		printf("tmp = %d\n", tmp);
	}
	if(hb == 10)
	{
	    return 100;
	}
	return hb;
}

void GetPresentHB(int savefd, char flag)
{
	int i = 0;
	char diff = 0;
	char buff[5] = {0};
//	DateTime  tmpDatime = {0};
	if(flag == HB_SUCCESS)
	{
		if(HB_DateTime_cnt > 29)
		{
			for(i = 29; i >= 0; i--)
			{
				if(i== 0){
					HB_DateTime[0] = get_DateTime();
					break;
				}
				HB_DateTime[i] = HB_DateTime[i-1];
			}
		}
		if(HB_DateTime_cnt <=29)
		{
			HB_DateTime[29-HB_DateTime_cnt] = get_DateTime();
		}
	}	
	else
	{
		// 与后台连接失败
 	}

	printf("HB_cnt= %d\n", HB_DateTime_cnt);
	DateTime tmpDatime  = get_DateTime();	
	if((diff = diff_time(HOUR, &HB_DateTime[29], &tmpDatime)) < 1 && HB_DateTime_cnt >=5 )
	{
//		printf("hour diff = %d\n", diff);
		for(i = 0; i<HB_DateTime_cnt; i++)
		{
			if(diff_time(SECOND, &HB_DateTime[29-i], &tmpDatime) < HB_DateTime_cnt){
//				printf("second diff = %d\n", diff);
				break;
			}
//			printf("second diff = %d\n", diff);
		}

		// 计算心跳率
		diff = get_HB((HB_DateTime_cnt -i)/(HB_DateTime_cnt * 100.0));
		printf("###########################i = %d, 心跳率:%d%%\n", i, diff);
		sprintf(buff, "%3d", diff);
		lseek(savefd, SEEK_SET, 0);
		write(savefd, buff, strlen(buff));
	}

	if(flag == HB_SUCCESS)
	{	
		if(HB_DateTime_cnt <=29)	
			HB_DateTime_cnt++;
	}
}


int lockfile(int fd)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 20;

	return fcntl(fd, F_SETLK, &lock);
}
// 文件锁防止该守护进程有多个同时运行，以后使用cron守护进程定时重启时用
int already_running(void)
{
	int fd;
	char buff[16] = {0};
	if( (fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE)) < 0)
	{
		syslog(LOG_ERR, "can't open %s: %s", LOCKFILE, strerror(errno));
		exit(1);
	}
	if(lockfile(fd) <0)
	{
		if(errno == EAGAIN || errno == EACCES)
		{
			close(fd);
			return 1;
		}
		syslog(LOG_ERR, "can't lock %s: %s", LOCKFILE, strerror(errno));
		exit(1);
	}

	ftruncate(fd, 0);
	sprintf(buff, "%ld", (long)getpid());
	write(fd, buff, strlen(buff)+1);
	return 0;
}


// 守护进程创建函数
void daemonize(const char *cmd)
{
	int			i, fd0, fd1, fd2;
	pid_t			pid;
	struct	rlimit		rl;
	struct	sigaction	sa;
	
	umask(0);
	
	if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		perror("can't get file limit");
		exit(1);
	}

	if((pid = fork()) < 0)
	{
		perror("can't fork");
		exit(1);
	}else if(pid != 0) // parent
	{
		exit(0);
	}

	setsid();

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGHUP, &sa, NULL) < 0)
	{
		perror("can't ignore SIGHUP");
		exit(1);
	}
	
	if((pid = fork()) < 0)
	{
		perror("can't fork");
		exit(1);
	}else if(pid != 0)	// parent
	{
		exit(0);
	}
	
	if(chdir("/") < 0)
	{
		perror("can't chdir");
		exit(1);
	}
	
	if(rl.rlim_max == RLIM_INFINITY)
	{
		rl.rlim_max = 1024;
	}

	for(i=0; i< rl.rlim_max; i++)
		close(i);

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);
	
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if(fd0 !=0 || fd1 != 1 || fd2 != 2){
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
		exit(1);
	}
	syslog(LOG_INFO, "%s is  ok.......", cmd);
}




