
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "include/ev_command_file.h"
#include "include/err.h"
#include "include/serv_config.h"
void  evdoit_l_serv(void);

static void daemonize(const char *);
static int already_running(void);
static int print_help(void);
static int print_version(void);
static int set_daemon(void);
static int set_loadbalance(void);
static int doit_serv(void);
static int set_stop(void);
static int set_start(void);
static int set_restart(void);
static int set_process_lock(void);


extern  int opt_type;
static  char * this_process_name = NULL;
static ev_command_t run_cmd[] = {
        {OPTION_HELP,        "-help",        print_help      },
        {OPTION_VERSION,     "-v",           print_version   },
        {OPTION_LOADBALANCE, "-load",        set_loadbalance },
        {OPTION_STOP,        "-stop",        set_stop        },
        {OPTION_START,       "-start",       set_start       },
        {OPTION_RESTART,     "-restart",     set_restart     },
        {OPTION_DEBUG,       "-debug",       NULL            },
        {OPTION_PROCESSLOCK, "NULL",         set_process_lock},
        {OPTION_DAEMON,      "-daemon",      set_daemon      },
        {OPTION_RUNNING,      NULL,          doit_serv     },
        {OPTION_NULL,        NULL,           NULL            }
};

void command_init_for_run(int type, int mask)
{
        ev_command_t *cmd = run_cmd;
        
        type &= mask;
        while ( cmd->type != OPTION_NULL || (cmd->name != NULL)) {
                if ((cmd->type & type) && cmd->start != NULL)
                {
                       printf("cmd->type:%d, %s\n", cmd->type, cmd->name);
                       cmd->start();
                }
                cmd++;
        }
}


int load_command_line(int argc, char **argv)
{
    int  config_type = OPTION_NULL, have_option;
    char **arg;
    ev_command_t *cmd;
    
     this_process_name = *argv;
    
     if (argc == 1){
         config_type = OPTION_RUNNING;
         config_type |= OPTION_PROCESSLOCK;
         return config_type;
     }

    arg = argv + 1;
    have_option = 0;    
    do {
           if (!strcmp(*arg, "-help")){ 
               config_type = OPTION_HELP;
               break;
           }
           if (!strcmp(*arg, "-v")){
               config_type = OPTION_VERSION;
               break;
           }
           if (!strcmp(*arg, "-start")){
               config_type |= OPTION_START;
               break;
           }
           if (!strcmp(*arg, "-stop")){
               config_type = OPTION_STOP;
               break;
           }
           if (!strcmp(*arg, "-restart")){
               config_type |= OPTION_RESTART;
               break;
           }
           if (!strcmp(*arg, "-daemon") && !(config_type & OPTION_DEBUG)) {
               config_type |= OPTION_DAEMON;
               arg++;
                continue;
           }
           if (!strcmp(*arg, "-debug") && !(config_type & OPTION_DAEMON)) {
               config_type |= OPTION_DEBUG;
               arg++;
                continue;
           }
           if (!strcmp(*arg, "-load")) {
               config_type |= OPTION_LOADBALANCE;
               arg++;
               continue;
           }
           have_option++;
    } while(*++arg);
    if ( have_option > 0)
         config_type = OPTION_HELP;
    config_type |= OPTION_PROCESSLOCK;
    config_type |= OPTION_RUNNING;
     printf("config_type:%d\n", config_type);

    return config_type;
}


// 命令行执行函数
static int set_restart(void)
{
        return 0;
}
static int set_stop(void)
{
    struct flock lock;
    int fd;
    pid_t pid;
    char buff[10] = {0};

    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = F_WRLCK;

    if( (fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE)) < 0){
         perror("open failed"); 
         exit(EXIT_FAILURE);
    }
    if ( read(fd, buff,  sizeof(buff)) <= 0){
        close(fd);
        perror("read failed");
        exit(EXIT_FAILURE);
    }
    pid = (pid_t)atoi(buff);
    if ( fcntl(fd, F_GETLK, &lock) < 0){
         close(fd);
         perror("fcntl failed");
         exit(EXIT_FAILURE);
    }
    if (lock.l_type != F_UNLCK){
        if (kill(pid, SIGKILL) < 0)
            perror("kill failed"), exit(EXIT_FAILURE);
        printf("kill [%d] success\n", (int)pid);
    }
    close(fd);
    exit(EXIT_SUCCESS);
}
static int set_start(void)
{
        return 0;
}


static int doit_serv(void)
{
        printf("running doit_l_serv");
        evdoit_l_serv();
        return EXIT_SUCCESS;
}

static int set_loadbalance(void)
{
        pthread_t  tid;
        int err;

        if ( (err = pthread_create(&tid, NULL, load_balance_pthread, NULL)) < 0 ){
                printf("pthread_create failed ...");
                exit(EXIT_FAILURE);
        }
        return EXIT_SUCCESS;
}

static int set_process_lock(void)
{
        if (already_running()){
            if (opt_type & OPTION_DAEMON)
              syslog(LOG_ERR, "%s is already running, %s[%d] exit ...", this_process_name, this_process_name, (int)getpid());
                exit(EXIT_FAILURE);
        }
        return EXIT_SUCCESS;
}
static int set_daemon(void)
{
       daemonize(this_process_name);
       return EXIT_SUCCESS; 
}

static int print_version(void)
{
        printf("vsersion:1.01\n");
        printf("copyright 2016,shenzhen evpower group company own ...\n");
        exit(EXIT_SUCCESS);
}

static int print_help(void)
{
        printf("usage: %s [option]  ( %s -daemon for default)\n", this_process_name, this_process_name);
        printf("option: -help    printf help information\n");
        printf("        -v       printf program version\n");
        printf("        -daemon  set daemonize for program\n");
        printf("        -load    set loadblance for program\n");
        printf("        -long    set long connect for tcp, default set short connect\n");
        printf("        -start   set process start\n");
        printf("        -stop    set process stop\n");
        printf("        -restart set restart process\n");
        printf("        -debug   set debug model\n");
        exit(EXIT_SUCCESS);
}

// 进程锁
 
static int lockfile(int fd)
{
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
       return fcntl(fd, F_SETLK, &lock);
}
// 文件锁防止该守护进程有多个同时运行，以后使用cron守护进程定时重启时用
static int already_running(void)
{
        int fd;
        char buff[20] = {0};
        if( (fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE)) < 0)
        {
               if (opt_type & OPTION_DAEMON)
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
              if (opt_type & OPTION_DAEMON)
              syslog(LOG_ERR, "can't lock %s: %s", LOCKFILE, strerror(errno));
              exit(EXIT_FAILURE);
        }
       ftruncate(fd, 0);
       sprintf(buff, "%ld", (long)getpid());
       write(fd, buff, strlen(buff)+1);
       return 0;
}



// 守护进程创建函数
static void daemonize(const char *cmd)
{
	int			fd0, fd1, fd2;
        unsigned int            i;
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

