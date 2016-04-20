


//基于TCP的EV充电桩与EV_Router通信 服务器端
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>

void daemonize(const char *cmd);

int main(int argc , char * argv[])
{
	//服务器初始化
	// 设置信号处理
	//5.响应

	daemonize("a.out");

	while(1) {}	
		system("echo 1  >> /home/jemore/aa");
}

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

	sleep(1);
	if(setsid() < 0)
		exit(1);

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
	
	sleep(1);
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
}



