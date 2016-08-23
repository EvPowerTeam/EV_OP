


#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>


void daemonize(const char *cmd)
{
	int			i, fd0, fd1, fd2;
	pid_t			pid;
	struct	rlimit		rl;
	struct	sigaction	sa;
	
	umask(0);
	
	if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		printf("can't get file limit");
		exit(1);
	}

	if((pid = fork()) < 0)
	{
		printf("can't fork");
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
		printf("can't ignore SIGHUP");
		exit(1);
	}
	
	if((pid = fork()) < 0)
	{
		printf("can't fork");
		exit(1);
	}else if(pid != 0)	// parent
	{
		exit(0);
	}
	
	if(chdir("/") < 0)
	{
		printf("can't chdir");
		exit(1);
	}
	
	if(rl.rlim_max == RLIM_INFINITY)
	{
		rl.rlim_max = 1024;
	}
    daemon_proc = 1;
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

int already_running(void)
{
	int fd;
	char buff[20] = {0};
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

int main(int argc, char **argv)
{
    daemonize(argv[0]);
    if (already_running())
    {
        syslog(LOG_ERR, "%s[%ld] is already running", argv[0], getpid());
		exit(1);
	}

    // 创建管道

    for ( ; ; )
    {
        
    
    
    }



    return 0;
}


