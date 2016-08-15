

#include "./include/net_info.h"
#include "./include/serv_config.h"
#include "./include/err.h"
#include "./include/system_call.h"
#include "./include/CRC.h"
#include "./include/AES.h"
#include "./include/serial.h"
//基于TCP的EV充电桩与EV_Router通信 服务器端
#include <libev/uci.h>
#include <libev/file.h>

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
#include <errno.h>
#include <dirent.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <mqueue.h>
#if 0
struct have_charge{
    char    way;
    char    index;
};
 struct have_charge have[10];
 int    get, put;
pthread_mutex_t  have_mutex;		// 客户端互斥锁
pthread_cond_t	 have_cond;		// 线程条件变量
pthread_mutex_t  have_mutex = PTHREAD_MUTEX_INITIALIZER;	// 线程互斥锁初始化
pthread_cond_t   have_cond = PTHREAD_COND_INITIALIZER;		// 线程条件变量初始化

#endif 
pthread_mutex_t		clifd_mutex = PTHREAD_MUTEX_INITIALIZER;	// 线程互斥锁初始化
//pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;		// 线程条件变量初始化
pthread_mutex_t		serv_mutex = PTHREAD_MUTEX_INITIALIZER;	// 线程互斥锁初始化
pthread_cond_t		serv_cond = PTHREAD_COND_INITIALIZER;		// 线程条件变量初始化
pthread_rwlock_t	charger_rwlock;

// 充电桩信息，数组
CHARGER_INFO_TABLE	ChargerInfo[10] = {0};  	// 用于存入每台电桩信息	
CHARGER_INFO_TABLE	*ChargerTable;			//没用

// 接收命令的数据结构
struct cb { time_t  start_time; time_t  end_time; };
struct rd 
{
    unsigned char   presentmode;//当前模式
    unsigned short  duration;   // 充电时间
    unsigned short  power;      // 电量
    unsigned short  chargingcode;//充电记录
    unsigned char   evid[16];
};

typedef struct  {
    unsigned char version[2];
    unsigned int cid;
    int          cmd;
    union  {
        struct  cb  chaobiao;
        struct  rd  record;
        char        name[20];
    }u;
}RECV_CMD;

// 待处理命令队列
struct  wait_task {
    int                 way;        // 页面设置方式还是服务器设置方式
    RECV_CMD            wait_cmd;        
    struct  wait_task   *next;
    struct  wait_task   *pre_next;
};
struct  wait_task   *wait_head = NULL;
struct  wait_task   *wait_tail = NULL;
pthread_mutex_t     wait_task_lock  = PTHREAD_MUTEX_INITIALIZER;    //保护待处理命令队列锁

// 已经处理完成命令队列
 typedef struct {
    unsigned int cid;
    int         cmd;
    int         err_code;
    int         chargercode;
    union {
        struct rd record;
        char   name[20];
    }u;
 }FINISH_TASK;

struct  finish_task {
    int                 way;
    FINISH_TASK         info; 
    struct  finish_task *next;
    struct  finish_task *pre_next;
};
struct  finish_task     *finish_head = NULL;
struct  finish_task     *finish_tail = NULL;
pthread_mutex_t     finish_task_lock = PTHREAD_MUTEX_INITIALIZER;   //保护已经完成命令队列锁
pthread_cond_t      finish_task_cond = PTHREAD_COND_INITIALIZER;   //保护已经完成命令队列锁


struct  finish_task *finish_task_create(int way, FINISH_TASK data)
{
    struct  finish_task     *task;

    if ( (task = (struct finish_task *)malloc(sizeof(struct finish_task))) == NULL)
        err_quit("malloc failed");
    task->way  = way;
    task->info = data;
    task->next = NULL;
    task->pre_next = NULL;
    return task;
}

void finish_task_add(int way, FINISH_TASK  cmd)
{
    struct finish_task  *task;
    task = finish_task_create(way, cmd); 
    pthread_mutex_lock(&finish_task_lock);
    task->pre_next = finish_tail;
    if (finish_tail == NULL)
    {
        finish_head = task;
        finish_tail = task;
    }
    else
    {
        finish_tail->next = task;
        finish_tail = task;
    }
    pthread_mutex_unlock(&finish_task_lock);
    pthread_cond_signal(&finish_task_cond);
}

struct finish_task *finish_task_remove_cid(unsigned int cid)
{
    struct finish_task *task = NULL;

    pthread_mutex_lock(&finish_task_lock);
    task = finish_head;
    while(task)
    {
        if (task->info.cid == cid)
        {
            if (task->next == NULL)
                finish_tail = task->pre_next;
            else
                task->next->pre_next = task->pre_next;
            if (task->pre_next == NULL)
                finish_head = task->next;
            else
               task->pre_next->next = task->next;
            break;
        }
        task = task->next;
    }
    pthread_mutex_unlock(&finish_task_lock);
    return task;
}

void finish_task_remove_head(struct finish_task *task)
{
    if (task->next == NULL)
        finish_tail = task->pre_next;
    else
        task->next->pre_next = task->pre_next;

    if (task->pre_next == NULL)
        finish_head = task->next;
    else
        task->pre_next->next = task->next;
}

// create a struct wait_task node
struct  wait_task *wait_task_create(int way, RECV_CMD  cmd)
{
    struct  wait_task   *task;

    if ( (task = (struct wait_task *)malloc(sizeof(struct wait_task))) == NULL)
        err_quit("malloc failed");
    // initialize
    task->way = way;
    task->wait_cmd = cmd;
    task->next = NULL;
    task->pre_next = NULL;
    return task;
}
void  wait_task_add(int way, RECV_CMD cmd)
{
    struct wait_task  *task;
    
    task = wait_task_create(way, cmd); 
    pthread_mutex_lock(&wait_task_lock);
    task->pre_next = wait_tail;
    if (wait_tail == NULL)
    {
        wait_head = task;
        wait_tail = task;
    }
    else
    {
        wait_tail->next = task;
        wait_tail = task;
    }
    pthread_mutex_unlock(&wait_task_lock);
}

struct wait_task *wait_task_remove_cid(unsigned int cid)
{
    struct wait_task *task = NULL;
    pthread_mutex_lock(&wait_task_lock);
    task = wait_head;
    while(task)
    {
        if (task->wait_cmd.cid == cid)
        {
            if (task->next == NULL)
                wait_tail = task->pre_next;
            else
                task->next->pre_next = task->pre_next;
            if (task->pre_next == NULL)
                wait_head = task->next;
            else
               task->pre_next->next = task->next;
            break;
        }
        task = task->next;
    }
    pthread_mutex_unlock(&wait_task_lock);
    return task;
}

struct wait_task *wait_task_remove_cmd(unsigned int cid, int cmd)
{
    struct wait_task *task = NULL;
    printf("aa\n");
    pthread_mutex_lock(&wait_task_lock);
    printf("aaa\n");
    task = wait_head;
    while (task)
    {
        printf("aaaa\n");
        if (task->wait_cmd.cmd == cmd && task->wait_cmd.cid == cid)
        {
            if (task->next == NULL)
                wait_tail = task->pre_next;
            else
                task->next->pre_next = task->pre_next;
            if (task->pre_next == NULL)
                wait_head = task->next;
            else
               task->pre_next->next = task->next;
            break;
        }
        task = task->next;
    }
   pthread_mutex_unlock(&wait_task_lock);
   return task;
}


unsigned char average_current_compare;
typedef struct {
    unsigned char   total_num;              //充电装总个数，默认给8
    unsigned char   limit_max_current;	//限制的最大的充电电流
    unsigned char   have_update_file_flag;  //有更新文件标志
    unsigned char   tell_have_update_file_flag; //服务器告诉程序有更新文件标志位
    unsigned char   have_configuration_file_flag;//有配置文件标志
    unsigned char   present_charger_cnt;	//当前
    unsigned char   present_off_net_cnt;	//当前断网的个数
    unsigned char   present_networking_cnt;	//当前联网的总数
    unsigned char   present_charging_cnt;	//当前正在充电的个数
    unsigned int    present_record_cnt;	//当前充电记录个数
    int             have_powerbar_serial_fd;            //打开与灯板通信的串口描述符
    int             timeout_cnt;
    char            file_name[20];
    unsigned char   version[2];
//	pthread_mutex_t clifd_mutex;
//	pthread_cond_t	clifd_cond;
//	pthread_mutex_t serv_cond;
//	pthread_rwlock_t charger_rwlock;
}CHARGER_MANAGER;
CHARGER_MANAGER		charger_manager = {
	.total_num = 8,
	.limit_max_current = 56,
	.present_off_net_cnt = 8,
};

sigset_t        mask;

// 函数声明
void thread_make(int i);
void protocal_init_head(unsigned char CMD, unsigned char *p_str, unsigned int cid);
int  sock_serv_init(void);
void * thr_fn(void * arg);
int  charger_serv(int fd, unsigned char *Index);
void daemonize(const char *cmd);
void msleep(unsigned long mSec);
int already_running(void);
void print_PresentMode(unsigned char pmode);
void print_SubMode(unsigned short submode);
void print_SelectSocket(unsigned char socket);
void key_init(unsigned char *key);
void clean_0x34_uci_database(unsigned char);
void  charger_info_init(int select);
void *pthread_listen_program(void * arg);
void *thread_main(void * arg);
void *pthread_service_send(void *arg);
void *pthread_service_receive(void *arg);
void *sigle_pthread(void *arg);
void *load_balance_pthread(void *arg);
int charger_caobiao_handler(int fd, unsigned short recordid, unsigned short chargercode, \
                            unsigned int StartTime, unsigned int EndTime, unsigned char index);
#define     CHARG_FILE	"/etc/config/chargerinfo"
#define		UPDATE_FILE	"/etc/config/CNMB.bin"
#define     SERVER_FILE "/etc/config/chargerinfo"
#define     POWER_BAR_SERIAL    "/dev/ttyUSB0"
#define     WORK_DIR            "/mnt/umemory/power_bar"
#define     CHAOBIAO_DIR        "DATA"      // 存抄表记录文件，以CID命名
#define     UPDATE_DIR          "UPDATE"    // 存更新包文件，以CNMB.bin命名
#define     CONFIG_DIR          "CONFIG"    // 存推送配置文件,以CID命名
#define     RECORD_DIR          "RECORD"
#define     CONFIG_FILE         "/etc/chargerserver.conf"
#define     FILEPERM            (S_IRUSR | S_IWUSR)
#define     WAIT_CMD_NONE       0x00
#define     WAIT_CMD_CHAOBIAO   0x01
#define     WAIT_CMD_ALL_CHAOBIAO   0x07
#define     WAIT_CMD_ONE_UPDATE 0x02
#define     WAIT_CMD_ALL_UPDATE 0x05
#define     WAIT_CMD_CONFIG     0x03
#define     WAIT_CMD_YUYUE      0x04
#define     WAIT_CMD_UPLOAD     0x06    //上传充电数据
#define     WEB_WAY             0x01
#define     SERVER_WAY          0x02    // 后台模式
// direction initializa
void  dir_init(void)
{
    char    name[100];
    mkdir(WORK_DIR, 0777);
    sprintf(name, "%s/%s%c", WORK_DIR, CHAOBIAO_DIR, '\0');
    //创建抄表目录
    mkdir(name, 0777);
    sprintf(name, "%s/%s%c", WORK_DIR, CONFIG_DIR, '\0');
    mkdir(name, 0777);
    sprintf(name, "%s/%s%c", WORK_DIR, UPDATE_DIR, '\0');
    mkdir(name, 0777);
    sprintf(name, "%s/%s%c", WORK_DIR, RECORD_DIR, '\0');
    mkdir(name, 0777);
}
int readable_timeout(int fd, int sec)
{
	fd_set	rset;
	struct timeval	tv;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	//maxfdp=sock>fp?sock+1:fd+1;   
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	return (select(fd+1, &rset, NULL, NULL, &tv));
}
void sig_handler(int sig)
{
	printf("定时器被唤醒....\n");
}

#define M_ARENA_MAX     -8
int main(int argc , char * argv[])
{
	//服务器初始化
	// 设置信号处理
	//5.响应

	int                 i, connfd, listenfd, err;
	int                 maxfd = -1;
	struct sockaddr     *cliaddr;
	pthread_t	        tid;
	fd_set              rendezvous, rset;
	struct  sigaction   sa;

#if 0
        char    val_buff[200] = {0};
        char    send_buff[200] = {0};
        int     CID  = 75001152;
        char    *sptr;
        time_t  tm;
         memset(val_buff, 0, sizeof(val_buff));
        sprintf(val_buff, "/Charging/canStartCharging?");
        sprintf(val_buff+strlen(val_buff), "chargerID=%08d&", CID);
        memset(send_buff, 0, sizeof(send_buff));
        for (i = 0; i < 16; i++)
        {
            sprintf(send_buff + strlen(send_buff), "%02x", i);
        }
        
        sprintf(val_buff + strlen(val_buff), "privateID=%s&", send_buff);
        tm = time(0);
        sprintf(val_buff + strlen(val_buff), "startTime=%d&", tm);
        sprintf(val_buff + strlen(val_buff), "power=%d&", 0);
        sprintf(val_buff + strlen(val_buff), "status=%d&", 1);
        sprintf(val_buff + strlen(val_buff), "chargingRecord=%d&", 100);
        sprintf(val_buff + strlen(val_buff), "mac=%s&", "FF:93:93:33:FF:44");
        sprintf(val_buff + strlen(val_buff), "chargingType=%d", 2);
        // 发送
        while (1)
        {
            api_send_buff("http", "10.9.8.2:8080/ChargerAPI", val_buff, "", NULL, NULL);
            sleep(3);

	}

#endif



    mallopt(M_ARENA_MAX, 1);
#if  1
	daemonize(argv[0]);
	if(already_running())
	{
		syslog(LOG_ERR, "%s[%ld] is already running", argv[0], getpid());
		exit(1);
    }
#endif
	if (access(CHARG_FILE,  F_OK) != 0)
	{
        file_trunc_to_zero(CHARG_FILE); //CHARG_FILE);	//创建/etc/config/chargerinfo文件
        ev_uci_add_named_sec("chargerinfo.%s=tab", "TABS");
//		ev_uci_add_named_sec("chargerinfo.%s=rod", "Record");
        ev_uci_add_named_sec("chargerinfo.%s=issued", "SERVER");
        ev_uci_add_named_sec("chargerinfo.%s=respond", "CLIENT");
		charger_info_init(1); // 初始化数据库数据 
	} else 	
		charger_info_init(0); //数据库中读入信息，写入内存数组中，如果没有，也直接运行。(CID, IP, KEY)
	

#if  0
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0)
        err_sys("sigction failed");
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGALRM);
    if ( (err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
        err_sys("pthread_sigmask failed");
#endif 

    dir_init(); // 创建初始化目录
//    if ( (charger_manager.have_powerbar_serial_fd  = UART_Open(POWER_BAR_SERIAL)) < 0)
//        err_sys("open power bar serial error");
//    if ( UART_Init(charger_manager.have_powerbar_serial_fd, 9600, 0, 8, 1, 'N') < 0)
//        err_sys("init power bar serial error error");
#if 0
    if (access(SERVER_FILE, F_OK) != 0)
    {
        init_file(SERVER_FILE);
        ev_uci_add_named_sec("chargerinfo.%s=issued", "SERVER");
        ev_uci_add_named_sec("chargerinfo.%s=respond", "CLIENT");
    }
#endif
	socklen_t addrlen, clilen;
#if 1
    FD_ZERO(&rendezvous);
	listenfd = sock_serv_init();
    FD_SET(listenfd, &rendezvous);
    maxfd = listenfd + 1;
	addrlen = sizeof(struct sockaddr_in);
	// 线程相关初始化
//	if ((err = pthread_create(&tid, NULL, &pthread_listen_program, (void *)0)) < 0)
//		errno = err, err_sys("pthread_create error");
	if ((err = pthread_create(&tid, NULL, &load_balance_pthread, (void *)0)) < 0)
		errno = err,err_sys("pthread_create error");
	
    if( (err = pthread_create(&tid, NULL, &pthread_service_send, (void *)0)) < 0)
		errno = err, err_sys("pthread_create error");
    
    if( (err = pthread_create(&tid, NULL, &pthread_service_receive, (void *)0)) < 0)
		errno = err, err_sys("pthread_create error");
    
//    if ((err = pthread_create(&tid, NULL, signal_pthread, NULL)) < 0)
//        errno = err, err_sys("pthread_create failed");

	// 保护充电信息的读写锁，初始化
	if ((err = pthread_rwlock_init(&charger_rwlock, NULL)) < 0)
		errno = err, err_sys("pthread_rwlock_init error");
   syslog(LOG_INFO, "%s[%ld] is  initlizaton finish...", argv[0], getpid());
    for ( ; ; )
	{
//		clilen = addrlen;
//        if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0)
//            err_sys("select failed");
		if ((connfd = accept(listenfd, NULL, NULL)) < 0 )
		{
			if(errno == EINTR || errno == ECONNABORTED)
				continue;
			else
              err_ret("accept error");
		}
//		printf("conn fd = %d\n", connfd);
		if ( (err = pthread_create(&tid, NULL, &thread_main, (void *)connfd)) < 0)
			errno = err, err_sys("pthread_create error");
#if 0
        rset = rendezvous;
        if (select(maxfd, &rset, NULL, NULL, NULL) < 0)
            err_sys("select failed");
        if (FD_ISSET(i, &rset) )
        {
           if ((connfd = accept(listenfd, NULL, NULL)) < 0)
              err_ret("accept failed");
           pthread_create(&tid, NULL, &thread_main, (void *)connfd);
        }
#endif
    }
#endif	
	return 0;
}

// 线程处理函数
void * thread_main(void * arg)
{
	int connfd;
//	unsigned char	recv_buff[128] = {0};
	unsigned char	ChargerIndex;
	
	connfd = ((int)arg);
	if(pthread_detach(pthread_self()) < 0)
	{
        err_sys("pthread detach failed");
        close(connfd);
        return NULL;
	}
	// 执行服务程序
	charger_serv(connfd, &ChargerIndex);

	// 关闭连接
	shutdown(connfd, SHUT_RDWR);  
	close(connfd);
	pthread_exit((void *)0);
//	sock_close(connfd);
	return NULL;
}


// 充电协议处理程序
int  charger_serv(int fd, unsigned char *Index)
{
	time_t 		    tm =0;
	int             len, i, n, ops, err;
	unsigned char   ChargerCnt, CID_flag =0, CMD;
	unsigned char   /* *recv_buff, *send_buff, *val_buff; */ recv_buff[1024] = {0}, send_buff[1024] = {0}, val_buff[200] = {0};
	unsigned short	CRC = 0x1d0f, SubMode = 0, tmp_2_val = 0;
	unsigned int	declen = 0, enclen=0, CID = 0, tmp_4_val = 0;
    unsigned char   *sptr = NULL, *rptr = NULL, *file_buff = NULL, *key_addr = NULL;
    struct stat     *st = NULL;
    struct wait_task    *wait = NULL;
    struct finish_task  *finish = NULL;
    struct timespec abs_timeout;
    unsigned int prio;
    ssize_t queue;
    mqd_t mqd;
    char *msg;
    FINISH_TASK     task;
    
    // 协议初始化
	if(readable_timeout(fd, 20) == 0)
	{
		debug_msg("service progream timeout");
        return 0;
	}
	if( (len = read(fd, recv_buff,  sizeof(recv_buff))) <= 0 ) // 客户端关闭连接
	{
        debug_msg("read error");
		return 0;
	}
	ops = fcntl(fd, F_GETFL);
	if(ops < 0)
	{
        debug_msg("fcntl error");
        return 0;
	}
	ops = ops | O_NONBLOCK;
	if(fcntl(fd, F_SETFL, ops) < 0)
    {
        debug_msg("fcntl error");
		return 0;
    }
	CID = *(unsigned int *)(recv_buff+5);
    CMD = recv_buff[4];
	printf("################len:%d------------------->CMD = %#x, CID = %d, CNT=%d\n", len, recv_buff[4], CID, charger_manager.present_charger_cnt);//charger_manager.present_charger_cnt);
	// 根据CID查找充电信息表，索引
	  if( (err = pthread_rwlock_wrlock(&charger_rwlock)) < 0)
      {
           debug_msg("pthread_rwlock_wrlock error");
            errno = err, err_sys("pthread_rwlock_wrlock failed");
      } 
      ChargerCnt = charger_manager.present_charger_cnt;
	   for(i=0; i<ChargerCnt; i++)
	   {
		    if( memcmp(&ChargerInfo[i].CID, &CID, 4) == 0) // 判断CID是否相同
		    {
			    printf("找到的CID========>%d\n", ChargerInfo[i].CID);

				*Index = i;
				CID_flag = 1;
				break;	
				//continue;
		    }
	    }
	if(pthread_rwlock_unlock(&charger_rwlock) < 0)
    {
        debug_msg("pthread_rwlock_unlock error");
        exit(1);
    }
	// 数据处理, 希望接收数据达到一定的个数在处理，否则丢掉，减少解密处理
	// 1.数据解密,CRC判断，帧头判断略
	if( recv_buff[4] !=  0x10)// 找到匹配的
	{
        if (CID_flag != 1)
            return 1;
		ChargerInfo[(*Index)].free_cnt = 0;
		ChargerInfo[(*Index)].free_cnt_flag = 1;
		My_AES_CBC_Decrypt(ChargerInfo[(*Index)].KEYB, recv_buff+9, len-9, send_buff);
	}else
	{
		My_AES_CBC_Decrypt(KEYA, recv_buff+9, len-9, send_buff);
	}
	declen = len-9;
	len = 0;
	RePadding(send_buff, declen, recv_buff+9, &len);
	len +=9;
	CRC = getCRC(recv_buff, len-2);
	if(CRC !=  *(unsigned short *)(recv_buff+len-2))
	{
		debug_msg("CRC校验失败...\n");
		//出错处理
		return 0;
	}
    msleep(100);
    // 判断有没有抄表指令
    if(recv_buff[4] != 0x34 && recv_buff[4] != 0x10)
    {
       printf("wait_cmd:===>%d\n", ChargerInfo[(*Index)].wait_cmd);
       if ( ChargerInfo[(*Index)].wait_cmd == WAIT_CMD_NONE && (wait = wait_task_remove_cmd(CID, WAIT_CMD_CHAOBIAO)) )
       {
            debug_msg("有抄表命令, Cid =%d...", CID);
            ChargerInfo[(*Index)].wait_cmd = wait->wait_cmd.cmd;
            ChargerInfo[(*Index)].chaobiao_start_time = wait->wait_cmd.u.chaobiao.start_time;
           ChargerInfo[(*Index)].chaobiao_end_time = wait->wait_cmd.u.chaobiao.end_time;
            if(charger_caobiao_handler(fd, 0, 0, wait->wait_cmd.u.chaobiao.start_time, wait->wait_cmd.u.chaobiao.end_time, *Index))
            {
                if (wait->way == WEB_WAY)
                {
                    sprintf(val_buff, "%s/%s/%ld%c%s%c", WORK_DIR, CHAOBIAO_DIR, CID, '_', "web", '\0');
                } else
                {
                    sprintf(val_buff, "%s/%s/%ld%c%s%c", WORK_DIR, CHAOBIAO_DIR, CID, '_', "server", '\0');
                }
                if ( (ChargerInfo[(*Index)].chaobiao_fd = creat(val_buff, FILEPERM)) < 0)
                {
                    debug_msg("创建抄表文件失败, Cid = %d...", CID);
                }

            }
            goto clean;
       }
    }
	bzero(send_buff, sizeof(send_buff));
	// 电桩协议逻辑
    switch(recv_buff[4]){
		case	0x10:	// 连接请求
			// 用读的方式，锁住读写锁，然后查看信息,未实现
			printf("连接请求...\n");
			unsigned char tab_buff[20] = {0};
			if( (key_addr = (char *)malloc(16)) == NULL )
				goto clean;
			bzero(key_addr, 16);
            //获取随机KEYB值
			key_init(key_addr);	
			// 将接收到的CID
            // 一个断了的重新连接，只需要更新IP
			if(CID_flag == 1) 
			{
				//将改变的IP存入数据库
//				unsigned ip_buff[20] = {0};
				sprintf(val_buff, "%d.%d.%d.%d", recv_buff[9], recv_buff[10], recv_buff[11], recv_buff[12]);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.IP", ChargerInfo[(*Index)].tab_name) < 0)	// 存入IP
                    goto clean;
				memcpy(val_buff, key_addr, 16);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.KEYB", ChargerInfo[(*Index)].tab_name) < 0)
                    goto clean;
				bzero(val_buff, sizeof(val_buff));
				strncpy(val_buff, recv_buff+18, 10);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Model", ChargerInfo[(*Index)].tab_name) < 0) // series
                    goto clean;
				strncpy(val_buff, recv_buff+28, 10);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Series", ChargerInfo[(*Index)].tab_name) < 0)
                    goto clean;
				sprintf(val_buff, "%d.%02d%c", recv_buff[41], recv_buff[40], '\0');	//ChargerVersion
				ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerVersion", ChargerInfo[(*Index)].tab_name);
				
                bzero(val_buff, sizeof(val_buff));
				memcpy(ChargerInfo[(*Index)].IP, recv_buff+9, 4);
				memcpy(ChargerInfo[(*Index)].KEYB, key_addr, 16);
                ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
				strncpy(val_buff, recv_buff+18, 10);
				if(!strcmp(val_buff, "EVG-16N"))
				{
					ChargerInfo[(*Index)].model = EVG_16N;
				}else if(!strcmp(val_buff, "EVG-32N"))
				{
					ChargerInfo[(*Index)].model = EVG_32N;	
				}else if(!strcmp(val_buff, "EVG-32NW"))
				{
					ChargerInfo[(*Index)].model = EVG_32N;	
				}else if(!strcmp(val_buff, "EVG-32N"))
				{
					ChargerInfo[(*Index)].model = EVG_32N;	
				}
				if(ChargerInfo[(*Index)].free_cnt_flag == 0)
				{
					if(recv_buff[17] == CHARGER_CHARGING && ChargerInfo[(*Index)].flag == 0)  //解决服务器短暂断开重新连接
					{
						ChargerInfo[(*Index)].flag = 1;
						msleep(2000);		
					}
				}
			}
			else
			{
					//全新的连接，初始化key值
					// 更新数据库
                unsigned char   mac_addr[20] = {0};
                unsigned char   rwrite_flag = 0;
                unsigned char offset;
                rwrite_flag = 0; 
                sprintf(mac_addr, "%2x:%2x:%2x:%2x:%2x:%2x%c", recv_buff[42], recv_buff[43], recv_buff[44], recv_buff[45], recv_buff[46], recv_buff[47], '\0');
			    if( pthread_rwlock_wrlock(&charger_rwlock) < 0){
			        debug_msg("pthread_rwlock_wrlock error");
                    exit(1);
		    	}
				if( (sptr = find_uci_tables(TAB_POS)) == NULL)
				{
				}
				else
				{
                    for (i = 0; i<charger_manager.present_charger_cnt; i++)
                    {
                        debug_msg("mac = %s", ChargerInfo[i].MAC);
                        if(strncmp(ChargerInfo[i].MAC, mac_addr, strlen(mac_addr)) == 0)
                        {
                            offset = i;
                            rwrite_flag = 1;
                            sprintf(val_buff, "%d%c", CID, '\0');
                            debug_msg("===============================================同一个mac地址:%s", mac_addr);
				            if (ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CID", ChargerInfo[i].tab_name) < 0)
                            {
                                debug_msg("参数CID，写入数据库失败");
					            goto cmd_0x10;
                            }
                         //  break; 
                        }
                    
                    }
				}
				 if(rwrite_flag == 1)
                 {
//                     sprintf(tab_buff, "charger%d\0", i);
                       strcpy(tab_buff, ChargerInfo[offset].tab_name);
                 }
                 else
                 {
	    		    sprintf(tab_buff, "charger%d%c", charger_manager.present_charger_cnt+1, '\0');
                   // strcpy(sptr+strlen(sptr), tab_buff); //追加表名
                   if(sptr == NULL)
                   {
					    sptr = (unsigned char *)malloc(20);
					    memset(sptr, 0, 20);
                        sprintf(sptr, "%s", tab_buff);//追加表名
                   } else
                    sprintf(sptr+strlen(sptr), ",%s", tab_buff);//追加表名
                    printf("创建的表名为:%s\n", tab_buff);
				    if(ev_uci_save_action(UCI_SAVE_OPT, true, sptr, "%s", TAB_POS) < 0) // 保存数据到数据库
					    goto cmd_0x10;
                    debug_msg("save tab_name:%s", sptr);
				    ev_uci_add_named_sec("chargerinfo.%s=1", tab_buff);//创建changer表
                 }   
				// 保存CID，IP， KEYD到数据库
				sprintf(val_buff, "%d", CID);
				printf("tab_buff ===%s\n", tab_buff);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, mac_addr, "chargerinfo.%s.MAC", tab_buff )  < 0)
			        goto cmd_0x10;
				if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CID", tab_buff )  < 0)
					goto cmd_0x10;
				sprintf(val_buff, "%d.%d.%d.%d\0", recv_buff[9], recv_buff[10], recv_buff[11], recv_buff[12]);	// IP
				if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.IP", tab_buff )  < 0)
					goto cmd_0x10;
				memcpy(val_buff, key_addr, 16);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.KEYB", tab_buff)  < 0)
					goto cmd_0x10;
				bzero(val_buff, sizeof(val_buff));
				strncpy(val_buff, recv_buff+18, 10);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Model", tab_buff) < 0) // series
					goto cmd_0x10;
				strncpy(val_buff, recv_buff+28, 10);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Series", tab_buff) < 0)	
					goto cmd_0x10;
				bzero(val_buff, strlen(val_buff));
				sprintf(val_buff, "%d.%02d", recv_buff[41], recv_buff[40]);	//chargerVersion
				if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerVersion", tab_buff) < 0)	
					goto cmd_0x10;
				ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectCurrent", tab_buff);	
				ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentOutputCurrent", tab_buff);
				//	usleep(100);
				// 写入数据库
               if ( rwrite_flag !=1)
                    offset = charger_manager.present_charger_cnt;
                debug_msg("offset = %d\n", offset);
				ChargerInfo[offset].CID = CID;
				ChargerInfo[offset].IP[0] = recv_buff[9];
				ChargerInfo[offset].IP[1] = recv_buff[10];
				ChargerInfo[offset].IP[2] = recv_buff[11];
				ChargerInfo[offset].IP[3] = recv_buff[12];
                ChargerInfo[offset].wait_cmd = WAIT_CMD_NONE;
                strcpy(ChargerInfo[offset].MAC, mac_addr);
				memcpy(ChargerInfo[offset].KEYB, key_addr, 16);
				strcpy(ChargerInfo[offset].tab_name, tab_buff);
				strncpy(val_buff, recv_buff+18, 10);
				if(!strcmp(val_buff, "EVG-16N"))
				{
					ChargerInfo[offset].model = EVG_16N;
				}else if(!strcmp(val_buff, "EVG-32N"))
				{
					ChargerInfo[offset].model = EVG_32N;	
				}else if(!strcmp(val_buff, "EVG-32NW"))
				{
					ChargerInfo[offset].model = EVG_32N;	
				}
                for (i = 0; i <charger_manager.present_charger_cnt+1; i++)
                {
                    printf("%s   ",ChargerInfo[i].tab_name);
                }
				if(rwrite_flag == 0)
				    charger_manager.present_charger_cnt++;
			    if(pthread_rwlock_unlock(&charger_rwlock) < 0){
			        free(key_addr);
			        if(sptr != NULL)
				        free(sptr);
                    debug_msg("pthread_rwlock_unlock error");
                    exit(1);
                }
		  	 }
			// 发送0x11回应
			protocal_init_head(0x11, send_buff, CID);
			memcpy(send_buff+9, recv_buff+9, 4); 	//IP
			tm = time(0);			// 时间戳
			send_buff[13] = (unsigned char)(tm);
			send_buff[14] = (unsigned char)(tm >>8);
			send_buff[15] = (unsigned char)(tm >>16);
			send_buff[16] = (unsigned char)(tm >>24);
			memcpy(send_buff+17, key_addr, 16);	// KEYB
			CRC = getCRC(send_buff, CMD_0X11_LEN+2);	//CRC16
			send_buff[CMD_0X11_LEN+2] = (unsigned char)(CRC);
			send_buff[CMD_0X11_LEN+3] = (unsigned char)(CRC>>8);
			// 加密，发送
			Padding(send_buff+9, CMD_0X11_LEN-5, recv_buff, &enclen);
			My_AES_CBC_Encrypt(KEYA, recv_buff, enclen, send_buff+9);
#ifndef NDEBUG
			printf("发送加密数据...\n");
			for(i = 0; i< enclen+9; i++)
			{
				printf("%#x ", send_buff[i]);
			}
			printf("\n");
#endif
			if(write(fd, send_buff, enclen+9) != enclen+9)
			{
                debug_msg("write failed");
				goto clean;
			}
            goto clean;
cmd_0x10:
		 if(pthread_rwlock_unlock(&charger_rwlock) < 0){
            debug_msg("pthrea_rwlock_unlock failed");
            exit(1);
         }
			goto clean;
			// 发送回应
		break;
		case 	0x34:	// 心跳
			printf("心跳请求...\n");
//			if(ChargerInfo[(*Index)].model ==0)
//			{
//				send_buff[13] = 16;//16 ;//(ChargerInfo[(*Index)].model); //charger_manager.limit_max_current/1;//支持最大的电流
//				ChargerInfo[(*Index)].model = 16;//16;
//			}else
//			{
//					send_buff[13] = ChargerInfo[(*Index)].model;
//			}
			if(recv_buff[9] ==  CHARGER_CHARGING_COMPLETE_LOCK && ChargerInfo[(*Index)].flag == 0)  //解决充电完成，还未解锁导致浮点计算出错的BUG
			{
//				charger_manager.present_charging_cnt++;
				ChargerInfo[(*Index)].flag = 1;
			} 
			sprintf(val_buff, "%d%c", recv_buff[9], '\0');	//presentMode
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentMode", ChargerInfo[(*Index)].tab_name);
			ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.PresentOutputCurrent", ChargerInfo[(*Index)].tab_name);
			ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.SelectCurrent", ChargerInfo[(*Index)].tab_name);
			sprintf(val_buff, "%d%c", SubMode, '\0'); // SUB_MODE
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SubMode", ChargerInfo[(*Index)].tab_name);
//			 clean_0x34_uci_database(*Index); //将数据库电桩信息的数据字段置0
			// 心跳处理略
			//  回复心跳
//			bzero(recv_buff, sizeof(recv_buff));
        printf("wait_cmd = %d\n", ChargerInfo[(*Index)].wait_cmd);
     // 判断有没有命令处理
            if ( ChargerInfo[(*Index)].wait_cmd == WAIT_CMD_NONE && (wait = wait_task_remove_cid(CID)))
            {
//                printf("=========================================>后台有发送有命令等待处理\n");
                ChargerInfo[(*Index)].way = wait->way;
                ChargerInfo[(*Index)].wait_cmd_errcode = 0;
                ChargerInfo[(*Index)].wait_cmd = wait->wait_cmd.cmd;    //记录当前电桩正在执行的命令
                switch (wait->wait_cmd.cmd)
                {
                    case  WAIT_CMD_ONE_UPDATE: //软件更新
                        sprintf(val_buff, "%s/%s/%s%c", WORK_DIR, UPDATE_DIR, wait->wait_cmd.u.name, '\0');
                        if (access(val_buff, F_OK) != 0)
                        {
                            debug_msg("update firmware is no exist");
                            goto clean;
                        }
                        if ( (ChargerInfo[(*Index)].update_file_fd = open(val_buff, O_RDONLY, 0444)) < 0)
                        {
                            debug_msg("open %s failed", val_buff);
                            goto clean;
                        }
                        if ( (st = (struct stat *)malloc(sizeof(struct stat))) == NULL)
                        {
                             debug_msg("malloc (struct stat) failed");
                             close(ChargerInfo[(*Index)].update_file_fd); 
                            goto clean;
                        }
				        if(fstat(ChargerInfo[(*Index)].update_file_fd, st) < 0)
                        {
                            debug_msg("fstat failed");
                            close(ChargerInfo[(*Index)].update_file_fd); 
                            goto clean;
                        }
                        ChargerInfo[(*Index)].update_file_length = st->st_size; 
			        	protocal_init_head(0x90, send_buff, CID);
				        send_buff[9] = wait->wait_cmd.version[0];
				        send_buff[10] = wait->wait_cmd.version[1];	//版本号
				        send_buff[11] = (unsigned char)ChargerInfo[(*Index)].update_file_length;
				        send_buff[12] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 8);
				        send_buff[13] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 16);
				        send_buff[14] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 24);
				        if(ChargerInfo[(*Index)].update_file_length%1024>0)
				        {
					        send_buff[15] = ChargerInfo[(*Index)].update_file_length / 1024 + 1;
				        }else
				        {
					        send_buff[15] = ChargerInfo[(*Index)].update_file_length / 1024;
				        }
				        CRC = getCRC(send_buff, 16);	
				        send_buff[16] = (unsigned char)CRC;		// CRC
				        send_buff[17] = (unsigned char)(CRC >> 8);
				        Padding(send_buff+9, 9, recv_buff, &enclen);
			            My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
		    	        if(write(fd, send_buff, enclen+9) != enclen+9){
                            debug_msg("write failed");
                            goto clean;
                        }
                        goto clean;
                    break;

                    case WAIT_CMD_CONFIG: //推送配置
                         sprintf(val_buff, "%s/%s/%s%c", WORK_DIR, CONFIG_DIR, wait->wait_cmd.u.name, '\0');
                         if (access(val_buff, F_OK) != 0)
                        {
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            debug_msg("config file:%s is no exist", val_buff);
                            goto clean;
                        }
                        debug_msg("检测到有配置文件...");
                        if ( (st = (struct st *)malloc(sizeof(struct stat))) == NULL)
                         {
                             debug_msg("malloc config (struct stat) failed");
                             ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                             goto clean;
                         }
                        if ( (ChargerInfo[(*Index)].config_file_fd = open(val_buff, O_RDONLY, 0444)) < 0)
                        {
                            debug_msg("open config:%s failed", val_buff);
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            goto clean;
                        }      
                        if (fstat(ChargerInfo[(*Index)].config_file_fd, st) < 0)
                        {
                            err_ret("fstat config error");
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            goto clean;
                        }
                        ChargerInfo[(*Index)].config_file_length = st->st_size; 
			            if ( (sptr = (unsigned char *)malloc(1500)) == NULL)
                        {
                            debug_msg("malloc config failed");
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            goto clean;
                        }
                        if ( (rptr = (unsigned char *)malloc(1500)) == NULL)
                        {
                            debug_msg("malloc config failed");
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            goto clean;
                        }
                        if ( (file_buff = (unsigned char *)malloc(1024)) == NULL )
                        {
                            debug_msg("malloc config failed");
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            goto clean;
                        }
			            if (lseek(ChargerInfo[(*Index)].config_file_fd, 0, SEEK_SET) < 0)
                        {
                            debug_msg("lseek config failed");
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            goto clean;
                        }
                        if ( (n = read(ChargerInfo[(*Index)].config_file_fd, file_buff, 1024)) <= 0)
                        {
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            debug_msg("read configfile  error");
                            goto clean;
                        }
                        protocal_init_head(0x51, sptr, CID);
                        // 分组长度
			            if (ChargerInfo[(*Index)].config_file_length % 1024 == 0)
                             sptr[9] = ChargerInfo[(*Index)].config_file_length / 1024;
                        else
                             sptr[9] = (ChargerInfo[(*Index)].config_file_length / 1024) + 1;
                        ChargerInfo[(*Index)].config_frame_size = sptr[9];
                        // 包长度
                        sptr[10] = (unsigned char)ChargerInfo[(*Index)].config_file_length;
                        sptr[11] = (unsigned char)(ChargerInfo[(*Index)].config_file_length >> 8);
                        sptr[12] = (unsigned char)(ChargerInfo[(*Index)].config_file_length >> 16);
                        sptr[13] = (unsigned char)(ChargerInfo[(*Index)].config_file_length >> 24);
                        // 当前包长度
                        sptr[14] = (unsigned char)n;
                        sptr[15] = (unsigned char)(n >> 8);
                        sptr[16] = (unsigned char)(n >> 16);
                        sptr[17] = (unsigned char)(n >> 24);
                        sptr[18] = (unsigned char)0;
                        memcpy(sptr+19, file_buff, n);
                        CRC = getCRC(sptr, n+19);
                        sptr[19+n] = (unsigned char)CRC;
                        sptr[20+n] = (unsigned char)(CRC >> 8);
			            Padding(sptr+9, 12+n, rptr, &enclen);
		                My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, rptr, enclen, sptr+9);
			            if(write(fd, sptr, enclen+9) != enclen+9){
                            debug_msg("write config error");
                            ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                            goto clean;
                        }
                        goto clean;
                    break;

                    case    WAIT_CMD_ALL_UPDATE:
                            i = 0;
                            char  tmp_buff[20] = {0};
                            for (i = 1; i <= ChargerCnt; i++)
                            {
                                if (ChargerInfo[i].wait_cmd == WAIT_CMD_ALL_UPDATE || ChargerInfo[i].wait_cmd == WAIT_CMD_NONE)
                                {
                                     // 读取版本号
		                            if(ev_uci_data_get_val(val_buff, 20, "chargerinfo.charger%d.ChargerVersion", i) < 0)
                                     {
                                          debug_msg("更新电桩,读取数据版本号出现问题..."); 
                                     } else
                                     {
                                         strcpy(tmp_buff, val_buff);
                                        if (wait->wait_cmd.version[0] > atoi(strtok(val_buff, ".")) ||  \
                                                (wait->wait_cmd.version[0] = atoi(strtok(tmp_buff, ".")) && \ 
                                                    wait->wait_cmd.version[1] > atoi(strtok(NULL, "."))))
                                        {
                                            ChargerInfo[i].tell_have_update_file_flag = 1;
                                            ChargerInfo[i].wait_cmd = WAIT_CMD_ALL_UPDATE;
                                            ChargerInfo[i].wait_cmd_errcode = 0;
                                            ChargerInfo[i].way = wait->way;
                                        } else
                                        {
                                            debug_msg("更新固件版本低于电桩版本...");
                                        }

                                     }
//                                    charger_manager.have_update_file_total_count++;
                                    
                                }
                            }
                            sprintf(val_buff, "%s/%s/%s%c", WORK_DIR, UPDATE_DIR, wait->wait_cmd.u.name, '\0');
                            strcpy(charger_manager.file_name, wait->wait_cmd.u.name);
                            debug_msg("update file path:%s\n", val_buff);
                            charger_manager.version[0] = wait->wait_cmd.version[0];
                            charger_manager.version[1] = wait->wait_cmd.version[1];
                    break;

                    case    WAIT_CMD_YUYUE:
#if 0
                            ChargerInfo[(*Index)].yuyue_finish_flag =0;
				            protocal_init_head(0x34, send_buff, CID);
                            send_buff[9] =  (unsigned char)task->wait_cmd.u.yuyue.time;
                            send_buff[10] = (unsigned char)(task->wait_cmd.u.yuyue.time >> 8);
				            Padding(send_buff+9, 20, recv_buff, &enclen);
			                My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
		    	            if(write(fd, send_buff, enclen+9) != enclen+9){
                                return 0;
                            }
                            return 1;
#endif
                            break;
                   case     WAIT_CMD_CHAOBIAO:

                            ChargerInfo[(*Index)].wait_cmd = wait->wait_cmd.cmd;
                            ChargerInfo[(*Index)].chaobiao_start_time = wait->wait_cmd.u.chaobiao.start_time;
                            ChargerInfo[(*Index)].chaobiao_end_time = wait->wait_cmd.u.chaobiao.end_time;
                            if(charger_caobiao_handler(fd, 0, 0, wait->wait_cmd.u.chaobiao.start_time, wait->wait_cmd.u.chaobiao.end_time, *Index))
                            {
                                if (wait->way == WEB_WAY)
                                {
                                    sprintf(val_buff, "%s/%s/%ld%c%s%c", WORK_DIR, CHAOBIAO_DIR, CID, '_', "web", '\0');
                                } else
                                {
                                   sprintf(val_buff, "%s/%s/%ld%c%s%c", WORK_DIR, CHAOBIAO_DIR, CID, '_', "server", '\0');
                                }
                                if ( (ChargerInfo[(*Index)].chaobiao_fd = creat(val_buff, FILEPERM)) < 0)
                                    debug_msg("open chaobiao error");
                            }
                            goto clean;
                   break;
                   default:
                            goto clean;
                   break;
                } // end switch
            
            } // end  wait_task_remove_cid  
#if 1
            // 全部更新软件
			if ( ChargerInfo[(*Index)].wait_cmd == WAIT_CMD_ALL_UPDATE)
			{
				debug_msg("检测到有更新包..............\n");
                sprintf(val_buff, "%s/%s/%s%c", WORK_DIR, UPDATE_DIR, charger_manager.file_name, '\0');
                if (access(val_buff, F_OK) != 0)
                {
                    debug_msg("access all update failed");
                    ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                    goto clean;
                }
                ChargerInfo[(*Index)].update_file_fd = open(val_buff, O_RDONLY, 0444);
                if ( (st = (struct stat *)malloc(sizeof(struct stat))) == NULL)
                 {
                        debug_msg("malloc (struct stat)  all update failed");
                       ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                       goto clean;
                 }
		        if (stat(val_buff, st) < 0)
                 {
                      debug_msg("stat  all update failed");
                       ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                      goto clean;
                 }
                ChargerInfo[(*Index)].update_file_length = st->st_size;

				protocal_init_head(0x90, send_buff, CID);
				send_buff[9] = charger_manager.version[0];
				send_buff[10] = charger_manager.version[1];	//版本号
				send_buff[11] = (unsigned char)ChargerInfo[(*Index)].update_file_length;
				send_buff[12] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 8);
				send_buff[13] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 16);
				send_buff[14] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 24);
				if(ChargerInfo[(*Index)].update_file_length%1024>0)
				{
					send_buff[15] = ChargerInfo[(*Index)].update_file_length / 1024 + 1;
				}else
				{
					send_buff[15] = ChargerInfo[(*Index)].update_file_length / 1024;
				}
				CRC = getCRC(send_buff, 16);	
				send_buff[16] = (unsigned char)CRC;		// CRC
				send_buff[17] = (unsigned char)(CRC >> 8);
				for(i = 0; i<18; i++)
				{
					printf("%#x ", send_buff[i]);
				}
				printf("\n");
				Padding(send_buff+9, 9, recv_buff, &enclen);
			    My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
		    	if(write(fd, send_buff, enclen+9) != enclen+9){
                       ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                       debug_msg("write all update failed");
                        goto clean;
                }
                ChargerInfo[(*Index)].wait_cmd_errcode = 0;
                goto clean;
			} 
#endif
				
                protocal_init_head(0x35, send_buff, CID);
				tm = time(0);
				send_buff[9] = (unsigned char)tm;	//时间戳
				send_buff[10] = (unsigned char)(tm>>8);
				send_buff[11] = (unsigned char)(tm>>16);
				send_buff[12] = (unsigned char)(tm>>24);
				CRC = getCRC(send_buff, CMD_0X35_LEN+2);	
				send_buff[14] = (unsigned char)CRC;		// CRC
				send_buff[15] = (unsigned char)(CRC >> 8);
				Padding(send_buff+9, CMD_0X35_LEN-5, recv_buff, &enclen);
			    //加密
			    My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
		    	if(write(fd, send_buff, enclen+9) != enclen+9){
			    	printf("子进程发送数据失败...\n");
			    	if(errno == EAGAIN)
				    {
					    printf("子线程没有准备好数据...\n");
				    }
			        goto clean;
            }
            ChargerInfo[(*Index)].wait_cmd_errcode = 0;
            if (ChargerInfo[(*Index)].wait_cmd != WAIT_CMD_ALL_UPDATE)
                ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
            ChargerInfo[(*Index)].flag = 0;
		    goto clean;	
		break;
		case	0x54:	//充电请求
			printf("充电请求...\n");
			pthread_mutex_lock(&serv_mutex);
		ChargerInfo[(*Index)].real_current = 0;
			ChargerInfo[(*Index)].flag = 0;
			ChargerInfo[(*Index)].cmd = 0x54;
            charger_manager.timeout_cnt = 0;
			// 统计充电请求个数
			//记录客户信息
			ChargerInfo[(*Index)].start_time = time(0);	//开始充电时间
			ChargerInfo[(*Index)].charging_code = *(unsigned short *)(recv_buff+33);			//赋值charging_code
			// load  balance 处理
			if( (recv_buff[29] !=  0 )&& (recv_buff[29]%16 == 0))
			{
				ChargerInfo[(*Index)].model = recv_buff[29];
			}
			ChargerInfo[(*Index)].real_current = 10;
            
#if 1
            while(ChargerInfo[(*Index)].real_current <= 0 && charger_manager.timeout_cnt <100)
		    {
				printf(".....................%d...................正在睡眠等待分配电流\n", ChargerInfo[(*Index)].real_current);
				msleep(300);
                charger_manager.timeout_cnt++;
//					return 0;
			}
            if(charger_manager.timeout_cnt >= 100 && ChargerInfo[(*Index)].real_current <= 0)
            {
                    // failure
                   ChargerInfo[(*Index)].cmd = 0;
                   pthread_mutex_unlock(&serv_mutex);
                   send_buff[14] = 0x02;
                   goto reply_to_charger;
            }
            ChargerInfo[(*Index)].flag = 1;
			ChargerInfo[(*Index)].cmd = 0;
#endif            
        // 发送数据到后台
        memset(val_buff, 0, sizeof(val_buff));
        sprintf(val_buff, "/Charging/canStartCharging?");
        sprintf(val_buff+strlen(val_buff), "chargerID=%08d'&'", CID);
        memset(send_buff, 0, sizeof(send_buff));
        for (i = 0; i < 16; i++)
        {
            sprintf(send_buff + strlen(send_buff), "%02x", recv_buff[35 + i]);
        }
        
        sprintf(val_buff + strlen(val_buff), "privateID=%s", send_buff);
        memcpy(ChargerInfo[(*Index)].ev_linkid, recv_buff+35, 16);
//        tm = time(0);
//        sprintf(val_buff + strlen(val_buff), "startTime=%d'&'", tm);
//        sprintf(val_buff + strlen(val_buff), "power=%d'&'", 0);
//        sprintf(val_buff + strlen(val_buff), "status=%d'&'", recv_buff[17]);
//        tmp_2_val = *(unsigned short *)(recv_buff + 33);
//        sprintf(val_buff + strlen(val_buff), "chargingRecord=%d'&'", tmp_2_val);
//        sprintf(val_buff + strlen(val_buff), "mac=%s'&'", ChargerInfo[(*Index)].MAC);
//        sprintf(val_buff + strlen(val_buff), "chargingType=%d", recv_buff[55]);
        // 发送
        cmd_frun("dashboard url_post 10.9.8.2:8080/ChargerAPI %s", val_buff);
        sptr = mqreceive_timed("/dashboard.checkin");
        if (!sptr)
        {
            ChargerInfo[(*Index)].cmd = 0;
            ChargerInfo[(*Index)].flag = 0;
		    pthread_mutex_unlock(&serv_mutex);
		    send_buff[14] = 0x02;
		    //tell charger not to wait
		    goto reply_to_charger;
        }
        printf("=================================================>str = %s\n", sptr);
        switch ( *sptr-48) 
        {
            case    SYM_Charging_Is_Starting: 
                    debug_msg("后台允许充电...");
                    ev_uci_save_action(UCI_SAVE_OPT, true, send_buff,
				       "chargerinfo.%s.privateID", ChargerInfo[(*Index)].tab_name);
			        send_buff[14] = 0x01;			// systemMessage
            break;
            case    SYM_Charging_Is_Stopping:
                    debug_msg("后台不允许充电...");
			        send_buff[14] = 0x02;			// systemMessage
            break;
            case    SYM_No_Access_Right:
                    debug_msg("没有权限...");
			        send_buff[14] = 0x03;			// systemMessage
            break;
            case    SYM_Not_Money_Charging:
                    debug_msg("余额不足");
                    send_buff[14] = 0x04;
            break;
            case    SYM_EV_Link_Not_Valid:
                    debug_msg("易冲卡无效");
                    send_buff[14] = 0x06;
            break;

            case   SYM_EV_Link_Is_Used:
                    debug_msg("易冲卡已经使用");
                    send_buff[14] = 0x07;
            break;
            default:
                   send_buff[14] = 0x02;
            break;
        }           
       
        if (send_buff[14] ==  SYM_Charging_Is_Starting)
        {
            // 向后台发送充电
            cmd_frun("dashboard url_post 10.9.8.2:8080/ChargerAPI %s", val_buff);
            free(sptr);
            sptr = mqreceive_timed("/dashboard.checkin");
        } else
        {
            ChargerInfo[(*Index)].cmd = 0;
            ChargerInfo[(*Index)].flag = 0;
            pthread_mutex_unlock(&serv_mutex);
		    goto reply_to_charger;
        }
//        send_buff[14] = 1;
        pthread_mutex_unlock(&serv_mutex);
        send_buff[35] = ChargerInfo[(*Index)].real_current;

#if 1
			sprintf(val_buff, "%d%c", recv_buff[17], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentMode", ChargerInfo[(*Index)].tab_name);
			tmp_2_val = *(unsigned short *)(recv_buff + 18);
			sprintf(val_buff, "%d%c", tmp_2_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SubMode", ChargerInfo[(*Index)].tab_name);
//			tmp_4_val = *(unsigned int *)(recv_buff + 22);
//			sprintf(val_buff, "%d%c", tmp_4_val, '\0');
//			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.AccPowerStart", ChargerInfo[(*Index)].tab_name);
//			sprintf(val_buff, "%d%c", recv_buff[28], '\0');	//select socket
//			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectSocket", ChargerInfo[(*Index)].tab_name);
			sprintf(val_buff, "%d%c", recv_buff[29], '\0');	//select current
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectCurrent", ChargerInfo[(*Index)].tab_name);
			sprintf(val_buff, "%d%c", recv_buff[30], '\0');	//presentoutputcurrent
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentOutputCurrent", ChargerInfo[(*Index)].tab_name);
			tmp_2_val = *(unsigned short *)(recv_buff+33);
			sprintf(val_buff, "%d%c", tmp_2_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargingCode", ChargerInfo[(*Index)].tab_name);
			sprintf(val_buff, "%d%c", recv_buff[55], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerWay", ChargerInfo[(*Index)].tab_name);
#endif
			ChargerInfo[(*Index)].real_current = send_buff[35];
			SubMode = *(unsigned short *)(recv_buff+18);
			tm = *(unsigned int *)(recv_buff+13); 
			struct tm * tim = localtime(&tm);
			//回应充电请求
			//protocal_init_head(0x55, send_buff, ChargerInfo[(*Index)].CID);
reply_to_charger:			
            protocal_init_head(0x55, send_buff, *(unsigned int *)(recv_buff+5));
			tm = time(0);
			send_buff[9] = (unsigned char)tm;	//时间戳
			send_buff[10] = (unsigned char)(tm>>8);
			send_buff[11] = (unsigned char)(tm>>16);
			send_buff[12] = (unsigned char)(tm>>24);
            // 以下信息不准确
			send_buff[13] = CHARGER_CHARGING;	// targetMode 状态改变
			send_buff[15] = (unsigned char)2880;
			send_buff[16] = (unsigned char)(2880 >> 8); //ChargingDuration
			send_buff[17] = (unsigned char)(10);	// chargingCode  充电记录
			send_buff[18] = (unsigned char)(10 >> 0);
			memcpy(send_buff+19, recv_buff+35, 16);	// EVlik 卡
			CRC = getCRC(send_buff, CMD_0X55_LEN+2);
			send_buff[36] = (unsigned char)CRC;
			send_buff[37] = (unsigned char)(CRC >> 8);
			bzero(recv_buff, sizeof(recv_buff));	
			Padding(send_buff+9, CMD_0X55_LEN-5, recv_buff, &enclen);
			My_AES_CBC_Encrypt((ChargerInfo[(*Index)].KEYB), recv_buff, enclen, send_buff+9);
			if(write(fd, send_buff, enclen+9) != enclen+9)
			{
				debug_msg("waite failed");
				goto clean;
			}
cmd_0x54:
		    goto clean;
		case	0x56:	//充电中状态更新
//			printf("充电中状态更新...\n");
			SubMode = *(unsigned short *)(recv_buff+18);
			ChargerInfo[(*Index)].flag = 1;

			ChargerInfo[(*Index)].real_time_current = recv_buff[34];
			if(ChargerInfo[(*Index)].real_current  ==  recv_buff[34])
			{
				ChargerInfo[(*Index)].change_0x56_flag = 1;
				ChargerInfo[(*Index)].present_current = ChargerInfo[(*Index)].real_current;
				ChargerInfo[(*Index)].no_match_current = 0;
			}
			else
			{
				ChargerInfo[(*Index)].change_0x56_flag = 0;
				ChargerInfo[(*Index)].no_match_current = recv_buff[34];

			}
			if(recv_buff[17] == CHARGER_CHARGING && ChargerInfo[(*Index)].flag == 0)  //解决服务器短暂断开重新连接
			{
				ChargerInfo[(*Index)].flag = 1;
//				Delay(2);
			}
			printf("================================================> recv_buff[34] = %d\n", recv_buff[34]);
			tmp_2_val = *(unsigned short *)(recv_buff+32);
#if 1
			if(tmp_2_val != ChargerInfo[(*Index)].charging_code)  //判断断了重新充电是否是新纪录，是则把原来的写入数据库
			{
					// 新充电用户
#if 0
//				charger_manager.present_record_cnt++;
					//将就的信息写入数据库
				sprintf(val_buff, "%d%c", charger_manager.present_record_cnt, '\0');
				ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.record.%s", "nums");
				sprintf(val_buff, "%ld,%ld,%ld,%d,%d,%s%c", CID, ChargerInfo[(*Index)].start_time, 
					0/*ChargerInfo[(*Index)].end_time*/, ChargerInfo[(*Index)].power,recv_buff[31],
					ChargerInfo[(*Index)].ev_linkidtmp, '\0');
				ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.record.num%d",
						   charger_manager.present_record_cnt );
				ChargerInfo[(*Index)].charging_code =  tmp_2_val;
				ev_uci_save_action(UCI_SAVE_OPT, true, tmp_2_val,
						   "chargerinfo.%s.ChargingCode",
						   ChargerInfo[(*Index)].tab_name);
#endif
            }
			sprintf(val_buff, "%d%c", recv_buff[17], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentMode", ChargerInfo[(*Index)].tab_name);
			sprintf(val_buff, "%d%c", recv_buff[22], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentOutputCurrent", ChargerInfo[(*Index)].tab_name);
			tmp_2_val = *(unsigned short *)(recv_buff+23);
			sprintf(val_buff, "%d%c", tmp_2_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Duration", ChargerInfo[(*Index)].tab_name);
			tmp_2_val = *(unsigned short *)(recv_buff+25);
			sprintf(val_buff, "%d%c", tmp_2_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Power", ChargerInfo[(*Index)].tab_name);
			sprintf(val_buff, "%d%c", recv_buff[31], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerWay", ChargerInfo[(*Index)].tab_name);
#endif
			protocal_init_head(0x35, send_buff, CID);
			tm = time(0);
			send_buff[9] = (unsigned char)tm;	//时间戳
			send_buff[10] = (unsigned char)(tm>>8);
			send_buff[11] = (unsigned char)(tm>>16);
			send_buff[12] = (unsigned char)(tm>>24);
			send_buff[13] = (unsigned char)ChargerInfo[(*Index)].real_current;
			printf("=================================================> send_buff[13] = %d\n", send_buff[13]);
			CRC = getCRC(send_buff, CMD_0X35_LEN+2);	
			send_buff[14] = (unsigned char)CRC;		// CRC
			send_buff[15] = (unsigned char)(CRC >> 8);
			//加密
			bzero(recv_buff, sizeof(recv_buff));
			Padding(send_buff+9, CMD_0X35_LEN-5, recv_buff, &enclen);
			My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
			if(write(fd, send_buff, enclen+9) != enclen+9){
			    debug_msg("write failed");
                goto clean;
            }
            goto clean;
		break;
		case	0x58:	//停止充电请求
			debug_msg("停止充电请求");
			ChargerInfo[(*Index)].end_time = time(0);				//获取充电结束时间
			ChargerInfo[(*Index)].power = *(unsigned short *)(recv_buff+37);	//获取结束充电的电量
			ChargerInfo[(*Index)].flag = 0;
			ChargerInfo[(*Index)].real_current = 0;
			ChargerInfo[(*Index)].change_0x56_flag = 0;
//			task.u.record.power = *(unsigned short *)(recv_buff+37);       //电量
//			task.u.record.duration = *(unsigned short *)(recv_buff+31);    // 充电时间
//			task.u.record.chargingcode = *(unsigned short *)(recv_buff+33);// 充电记录
//			task.u.record.presentmode = recv_buff[17];                     // presentdmode
//			task.cmd = WAIT_CMD_UPLOAD;
//			task.cid = CID;
//			finish_task_add(ChargerInfo[(*Index)].way, task);            // 加入

			// 发送数据给后台
			memset(val_buff, '\0', sizeof(val_buff));
			sprintf(val_buff, "/ChargerState/stopState?");
			sprintf(val_buff + strlen(val_buff), "key={chargers:[{chargerId:\\\"%08d\\\",", CID);
			memset(send_buff, 0, sizeof(send_buff));
            for (i = 0; i < 16; i++) {
                sprintf(send_buff + strlen(send_buff), "%02x", recv_buff[39 + i]);
            }
            sprintf(val_buff + strlen(val_buff), "privateID:\\\"%s\\\",", send_buff);
			tmp_2_val = *(unsigned short *)(recv_buff + 37);
			sprintf(val_buff + strlen(val_buff), "power:%d,", tmp_2_val);
			tmp_2_val = *(unsigned short *)(recv_buff + 33);
			sprintf(val_buff + strlen(val_buff), "chargingRecord:%d,", tmp_2_val);
			sprintf(val_buff + strlen(val_buff), "mac:\\\"%s\\\",", ChargerInfo[(*Index)].MAC);
			sprintf(val_buff + strlen(val_buff), "chargingType:%d,", recv_buff[59]);
			sprintf(val_buff + strlen(val_buff), "status:%d}]}", recv_buff[17]);
			debug_msg("len: %d", strlen(val_buff));
			cmd_frun("dashboard url_post 10.9.8.2:8080/ChargerAPI %s", val_buff);
		
			memset(val_buff, 0, strlen(val_buff));
			charger_manager.present_record_cnt++;	//充电记录增加
			ev_uci_delete( "chargerinfo.%s.privateID", ChargerInfo[(*Index)].tab_name);
			ev_uci_delete( "chargerinfo.%s.ChargerWay", ChargerInfo[(*Index)].tab_name);
			ev_uci_delete( "chargerinfo.%s.ChargingCode", ChargerInfo[(*Index)].tab_name);

            if (ChargerInfo[(*Index)].start_time != 0)
            {
			    sprintf(val_buff, "%08d,%ld,%ld,%d,%d,",  CID, ChargerInfo[(*Index)].start_time, ChargerInfo[(*Index)].end_time, ChargerInfo[(*Index)].power,recv_buff[59]);
                for (i = 0; i < 16; i++)
                {
                    sprintf(val_buff + strlen(val_buff), "%02x", ChargerInfo[(*Index)].ev_linkid[i]);
                }
                val_buff[strlen(val_buff)] = '\n';
                sprintf(send_buff, "%s/%s/%08d%c", WORK_DIR, RECORD_DIR, CID, '\0');
                FILE    *file;
                if ( (file = fopen(send_buff, "ab+")) == NULL)
                     goto replay_0x58;
                fwrite(val_buff, 1, strlen(val_buff), file);
                fclose(file);
            }
replay_0x58:
            sprintf(val_buff, "%d%c", recv_buff[17], '\0');
			//写入数据库，更新相应表信息
			//当前模式
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentMode", ChargerInfo[(*Index)].tab_name);
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short*)(recv_buff+18);
			sprintf(val_buff, "%d", tmp_2_val);
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SubMode", ChargerInfo[(*Index)].tab_name);
//			bzero(val_buff, strlen(val_buff));
//			tmp_4_val = *(unsigned int *)(recv_buff +22);
//			sprintf(val_buff, "%d", tmp_4_val);
//			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.AccPowerEnd", ChargerInfo[(*Index)].tab_name) < 0)
//				return 0;
//			bzero(val_buff, strlen(val_buff));
//			sprintf(val_buff, "%d", recv_buff[28]);
//			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectSocket", ChargerInfo[(*Index)].tab_name) < 0)
//				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[29]);
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectCurrent", ChargerInfo[(*Index)].tab_name);
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short*)(recv_buff+33);
			sprintf(val_buff, "%d", tmp_2_val);
			ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargingCode", ChargerInfo[(*Index)].tab_name);
//			charger_manager.present_charging_cnt--;
			SubMode = *(unsigned short *)(recv_buff+18);
			protocal_init_head(0x57, send_buff, CID);
			tm = time(0);
			send_buff[9] = (unsigned char)tm;	//时间戳
			send_buff[10] = (unsigned char)(tm>>8);
			send_buff[11] = (unsigned char)(tm>>16);
			send_buff[12] = (unsigned char)(tm>>24);
			send_buff[13] = (unsigned char)CHARGER_READY;
			send_buff[14] = 0x02;
//			memcpy(send_buff+15, recv_buff+39, 16);	// EVlik 卡
			memcpy(send_buff+15, recv_buff + 39, 16);	// EVlik 卡
			CRC = getCRC(send_buff, CMD_0X57_LEN+2);	
			send_buff[31] = (unsigned char)CRC;		// CRC
			send_buff[32] = (unsigned char)(CRC >> 8);
			bzero(recv_buff, sizeof(recv_buff));
			Padding(send_buff+9, CMD_0X57_LEN-5, recv_buff, &enclen);
			My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
			if(write(fd, send_buff, enclen+9) != enclen+9){
				if(errno == EAGAIN)
				{
					printf("子线程没有准备好数据...\n");
				}
				return 0;
			}
		break;
		case 	0x50:  //推送配置信息
			printf("接收配置文件请求...\n");
            if (recv_buff[10] == 1) // 推送完成
            {
//                ChargerInfo[(*Index)].config_finish_flag = 1;
                ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
                close(ChargerInfo[(*Index)].config_file_fd);
                task.cid = CID;
                task.cmd = WAIT_CMD_CONFIG;
                task.err_code = 0;
                finish_task_add(ChargerInfo[(*Index)].way, task);
                printf("发送信号成功...\n");
                ChargerInfo[(*Index)].wait_cmd_errcode = 0;
                sprintf(val_buff, "%s/%s/%d", WORK_DIR, CONFIG_DIR, CID);
                unlink(val_buff);
                goto clean;
            } 
//            if (ChargerInfo[(*Index)].config_file_fd == 0)
//                return 0;
			if ( (sptr = (unsigned char *)malloc(1500)) == NULL)
            {
                debug_msg("malloc config failed");
                ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                goto clean;
            }
            if ( (rptr = (unsigned char *)malloc(1500)) == NULL)
            {
                debug_msg("malloc config failed");
                ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                goto clean;
            }
            if ( (file_buff = (unsigned char *)malloc(1024)) == NULL )
            {
                debug_msg("malloc config failed");
                ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                goto clean;
            }
			if (lseek(ChargerInfo[(*Index)].config_file_fd, (recv_buff[9]+1)*1024, SEEK_SET) < 0)
            {
                debug_msg("lseek config failed");
                ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                goto clean;
            }
            if ( (n = read(ChargerInfo[(*Index)].config_file_fd, file_buff, 1024)) <= 0)
            {
                debug_msg("open config failed");
                if ( n = 0)
                    ChargerInfo[(*Index)].wait_cmd_errcode = 0;
                else
                    ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                goto clean;
            }
            protocal_init_head(0x51, sptr, CID);
            // 分组长度
			if (ChargerInfo[(*Index)].config_file_length % 1024 == 0)
                sptr[9] = ChargerInfo[(*Index)].config_file_length / 1024;
            else
                sptr[9] = (ChargerInfo[(*Index)].config_file_length / 1024) + 1;
            // 包长度
            sptr[10] = (unsigned char)ChargerInfo[(*Index)].config_file_length;
            sptr[11] = (unsigned char)(ChargerInfo[(*Index)].config_file_length >> 8);
            sptr[12] = (unsigned char)(ChargerInfo[(*Index)].config_file_length >> 16);
            sptr[13] = (unsigned char)(ChargerInfo[(*Index)].config_file_length >> 24);
            // 当前包长度
            sptr[14] = (unsigned char)n;
            sptr[15] = (unsigned char)(n >> 8);
            sptr[16] = (unsigned char)(n >> 16);
            sptr[17] = (unsigned char)(n >> 24);
            sptr[18] = (unsigned char)(recv_buff[9]+1);
            memcpy(sptr+19, file_buff, n);
            CRC = getCRC(sptr, n+19);
            sptr[19+n] = (unsigned char)CRC;
            sptr[20+n] = (unsigned char)(CRC >> 8);
			Padding(sptr+9, 12+n, rptr, &enclen);
			My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, rptr, enclen, sptr+9);
			if(write(fd, sptr, enclen+9) != enclen+9){
                debug_msg("write config failed");
                ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                goto clean;
            }
            ChargerInfo[(*Index)].wait_cmd_errcode = 0;
			printf("回应配置文件请求成功...\n");
                goto clean;
		break;
		case	0xa3:	//控制电桩回应
		break;
		case	0xa5:	//抄表回应
                printf("接收到抄表请求...\n");
                unsigned short cnt;
                if (recv_buff[986] == 0)
                {
                        cnt = (*(unsigned short *)(recv_buff + 982)) - 15;
                } else
                {
                    cnt =  0;
                }
                tm = *(unsigned int *)(recv_buff + 13) - 1;
                printf("开始的充电时间:%s  ", ctime(&tm));
                tm = *(unsigned int *)(recv_buff + 17);
                printf("结束的充电时间:%s\n", ctime(&tm));
                printf("接收数据条数:%d,发送条数:%d\n", *((unsigned short*)(recv_buff+982)), cnt);
               
                //写充电记录到文件

                if(recv_buff[981] > 0 && write(ChargerInfo[(*Index)].chaobiao_fd, recv_buff + 21, 64*recv_buff[981]) != 64*recv_buff[981])
                {
                    debug_msg("write chaobiao failed");
                    charger_caobiao_handler(fd, 0xFFFF, cnt, ChargerInfo[(*Index)].chaobiao_start_time , ChargerInfo[(*Index)].chaobiao_end_time, *Index);
                    ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                    goto clean;
                }
                // 抄表记录已经完成
                if(recv_buff[986] == 1)
                {
                    if (ChargerInfo[(*Index)].wait_cmd == WAIT_CMD_CHAOBIAO)
                        task.cmd = WAIT_CMD_CHAOBIAO;
                    else
                        task.cmd = WAIT_CMD_ALL_CHAOBIAO;
                    ChargerInfo[(*Index)].wait_cmd_errcode = 0;
                    ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
                    close(ChargerInfo[(*Index)].chaobiao_fd);
                    task.cid = CID;
                    task.chargercode = *((unsigned short *)(recv_buff + 984));
                    task.err_code = 0;
                    sprintf(val_buff, "%08d%c", CID, '\0');
                    strcpy(task.u.name, val_buff);
                    finish_task_add(ChargerInfo[(*Index)].way, task);
//                    ChargerInfo[(*Index)].chaobiao_finish_flag = 1;
                    charger_caobiao_handler(fd, 0xFFFF, cnt, ChargerInfo[(*Index)].chaobiao_start_time ,ChargerInfo[(*Index)].chaobiao_end_time, *Index);
                
                } else
                {
                    charger_caobiao_handler(fd, 0, cnt, ChargerInfo[(*Index)].chaobiao_start_time , ChargerInfo[(*Index)].chaobiao_end_time, *Index);
                } 
                ChargerInfo[(*Index)].wait_cmd_errcode = 0;
                printf("回应抄表请求成功...\n"); 
                goto clean;
                
        break;
		case	0x91:	//更新指令回应
                printf("接收到更新指令请求...\n");
                protocal_init_head(0x90, send_buff, CID);
                send_buff[9] = 2;//版本号
                send_buff[10] = 12;
                send_buff[11] = (unsigned char)ChargerInfo[(*Index)].update_file_length;// 更新包长度
                send_buff[12] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 8);
                send_buff[13] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 16);
                send_buff[14] = (unsigned char)(ChargerInfo[(*Index)].update_file_length >> 24);
                // 分组长度
                if (ChargerInfo[(*Index)].update_file_length % 1024 == 0)
                    send_buff[15] = (ChargerInfo[(*Index)].update_file_length / 1024);
                else 
                    send_buff[15] = (ChargerInfo[(*Index)].update_file_length / 1024) + 1;
                CRC = getCRC(send_buff, CMD_0X90_LEN + 2);
                send_buff[16] = (unsigned char)CRC;
                send_buff[17] = (unsigned char)( CRC >> 8);
			    bzero(recv_buff, sizeof(recv_buff));
			    Padding(send_buff+9, CMD_0X90_LEN-5, recv_buff, &enclen);
			    My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
			    if(write(fd, send_buff, enclen+9) != enclen+9){
                    ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                    goto clean;
                }
                printf("回应更新指令成功...\n");
                ChargerInfo[(*Index)].wait_cmd_errcode = 0;
                goto clean;
		break;
		case	0x95:	//更新软件回应	
			printf("接收更新软件请求...%d\n", recv_buff[11]);
			if ( (sptr = (unsigned char *)malloc(1500)) == NULL)
            {
                    debug_msg("malloc update failed");
                    ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                    goto clean;
            }
            if ( (rptr = (unsigned char *)malloc(1500)) == NULL)
            {
                    debug_msg("malloc update failed");
                    ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                    goto clean;
            }
            if ( (file_buff = (unsigned char *)malloc(1024)) == NULL )
            {
                    debug_msg("malloc update failed");
                    ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                    goto clean;
            } 
			//计算偏移量
			//memset(buff, 0, sizeof(buff))
			if(recv_buff[12] == 1)
            {            // 线程不安全，未作处理
                close(ChargerInfo[(*Index)].update_file_fd);
                 task.cmd = ChargerInfo[(*Index)].wait_cmd;
                 ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
                 task.cid = CID;
                 task.err_code = 0;
                 sprintf(val_buff, "%08d%c", CID, '\0');
                 strcpy(task.u.name, val_buff);
                 finish_task_add(ChargerInfo[(*Index)].way, task);
                 ChargerInfo[(*Index)].wait_cmd_errcode = 0;
//                ChargerInfo[(*Index)].update_finish_flag = 1; 
                    goto clean;
            }
            if ( (err = pthread_mutex_lock(&clifd_mutex)) < 0)
                    errno = err, err_sys("pthread_mutex_lock 0x95 error");
			lseek(ChargerInfo[(*Index)].update_file_fd, recv_buff[11]*1024, SEEK_SET);
            if( (n = read(ChargerInfo[(*Index)].update_file_fd, file_buff, 1024)) <=0)
			{
                 debug_msg("read update failed");
                if ( (err = pthread_mutex_unlock(&clifd_mutex)) < 0)
                        errno = err, err_sys("pthread_mutex_unlock 0x95 error");
                goto clean;
			}
            if ( (err = pthread_mutex_unlock(&clifd_mutex)) < 0)
                    errno = err, err_sys("pthread_mutex_unlock 0x95 error");
//			write(backup_fd, buff, sizeof(buff));
			if(n<1024)
				n = 1024;
			protocal_init_head(0x94, sptr, CID);
			sptr[9] = 1;
			sptr[10] = 3;
			sptr[11] = (unsigned char)n;
			sptr[12] = (unsigned char)(n>>8);
			sptr[13] = (unsigned char)(n>>16);
			sptr[14] = (unsigned char)(n>>24);
			sptr[15] = recv_buff[11];
			memcpy(sptr+16, file_buff, n);
			CRC = getCRC(sptr, 16+n);	
			sptr[16+n] = (unsigned char)CRC;		// CRC
			sptr[17+n] = (unsigned char)(CRC >> 8);
			printf("接收的数据为 n=%d：", n);
			for(i=0;i<18+n; i++)
			{
				printf("%#x ", sptr[i]);
			}
			printf("\n");
			Padding(sptr+9, 9+n, rptr, &enclen);
			My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, rptr, enclen, sptr+9);
			if(write(fd, sptr, enclen+9) != enclen+9){
                debug_msg("write update failed");
                ChargerInfo[(*Index)].wait_cmd_errcode = 1;
                goto clean;
			}
            ChargerInfo[(*Index)].wait_cmd_errcode = 0;
			printf("回应更新软件回应成功...\n");
            goto clean;
		break;
	} // end  switch

	return 1;	
clean:

    if (CMD != 0x10)
    {
        if (ChargerInfo[(*Index)].wait_cmd_errcode == 1)
         {
            ChargerInfo[(*Index)].wait_cmd_errcode = 0;
            switch(ChargerInfo[i].wait_cmd)
            {
                case WAIT_CMD_CONFIG:
                     close(ChargerInfo[(*Index)].config_file_fd);
                     ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
                     sprintf(val_buff, "%s/%s/%d", WORK_DIR, CONFIG_DIR, CID);
                     unlink(val_buff);
                break;
                case WAIT_CMD_ONE_UPDATE:
                     close(ChargerInfo[(*Index)].update_file_fd);
                    ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
                 break;
                 case WAIT_CMD_ALL_UPDATE:
                    close(ChargerInfo[(*Index)].update_file_fd);
                    ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
                break;
                case WAIT_CMD_CHAOBIAO:
                     close(ChargerInfo[(*Index)].chaobiao_fd);
                    ChargerInfo[(*Index)].wait_cmd = WAIT_CMD_NONE;
                break;
        
            }
         }
    }
    if (sptr != NULL)
        free(sptr);
    if (rptr != NULL)
        free(rptr);
    if (file_buff != NULL)
        free(file_buff);
    if (key_addr != NULL)
        free(key_addr);
    if (st != NULL)
        free(st);
    if (wait != NULL)
        free(wait);
    if (finish != NULL)
        free(finish);
//    malloc_trim(0);

}

/**
 *  功能：   抄表记录
 *  函数名： charger_record_handler
 *  参数：  fd,  StartTime , EndTime
 *  返回值: void
 */
int 
charger_caobiao_handler(int fd, unsigned short recordid, unsigned short chargercode,  
                        unsigned int StartTime, unsigned int EndTime, unsigned char index)
{
   unsigned char sptr[50], rptr[50];
   time_t       tm;
   int          enclen;
   unsigned short   CRC;

   printf("接收数据条数2:%d\n", chargercode);
   protocal_init_head(0xA4, sptr, ChargerInfo[index].CID);
   tm = time(0);
   sptr[9] = (unsigned char)tm;
   sptr[10] = (unsigned char)(tm >> 8);
   sptr[11] = (unsigned char)(tm >> 16);
   sptr[12] = (unsigned char)(tm >> 24);
   sptr[13] = (unsigned char)recordid;    //chargercode 目前写0
   sptr[14] = (unsigned char)(recordid >> 8);
   sptr[15] = (unsigned char)StartTime;
   sptr[16] = (unsigned char)(StartTime >> 8);
   sptr[17] = (unsigned char)(StartTime >> 16);
   sptr[18] = (unsigned char)(StartTime >> 24);
   sptr[19] = (unsigned char)(EndTime);
   sptr[20] = (unsigned char)(EndTime >> 8); 
   sptr[21] = (unsigned char)(EndTime >> 16); 
   sptr[22] = (unsigned char)(EndTime >> 24); 
   sptr[23] = (unsigned char)(chargercode);
   sptr[24] = (unsigned char)(chargercode >> 8);
   CRC = getCRC(sptr, 25);
   sptr[25] = (unsigned char)CRC;
   sptr[26] = (unsigned char)(CRC >> 8);
   Padding(sptr+9, 18, rptr, &enclen);
   My_AES_CBC_Encrypt(ChargerInfo[index].KEYB, rptr, enclen, sptr+9);
   if(write(fd, sptr, enclen+9) != enclen+9)
   {
        err_ret("write caobiao error");
        return 0;
   }
   printf("回应抄表记录成功...\n");
   return 1;
}

void protocal_init_head(unsigned char CMD, unsigned char *p_str, unsigned int cid)
{
	unsigned char len =0;
	strncpy(p_str, "EV<", 3);
	switch(CMD)
	{
		case 0x11: 	len = CMD_0X11_LEN;	break;	//连接确认
		case 0x35:	len = CMD_0X35_LEN;	break;	//Router心跳
		case 0x55:	len = CMD_0X55_LEN;	break;	//充电请求回应
		case 0x57:	len = CMD_0X57_LEN;	break;	//停止充电请求回应
		case 0xa2:	len = CMD_0XA2_LEN;	break;	//控制电桩指令
		case 0xa4:	len = CMD_0XA4_LEN;	break;	//抄表
		case 0x90:	len = CMD_0X90_LEN;	break;	//更新指令
//		case 0x94:	len = CMD_0X94_LEN;	break;	//更新软件
	}
	p_str[3] = len;
	p_str[4] = CMD;
	p_str[5] = (unsigned char)(cid);
	p_str[6] = (unsigned char)(cid>>8);
	p_str[7] = (unsigned char)(cid>>16);
	p_str[8] = (unsigned char)(cid>>24);
	return ;	
}

// 线程处理函数
void * thr_fn(void *arg)
{
//	SERV_INFO	info = *(SERV_INFO *)arg;
	int fd = *(int *)arg;
	unsigned char ip[20] = {0};
	char  buff[128] = {0};
	int len = 0;
	printf("sock fd = %d\n", fd);
//	strcpy(ip, inet_ntoa( *(struct in_addr*)&info.ip));
//	printf("pthread client ip %s \n", ip);

	if(pthread_detach(pthread_self()) < 0)
	{
		printf("线程分离失败...\n");
	}
	while(1)
	{
		write(fd, "hellworld\n", 10);

		if( (len = read(fd, buff, sizeof(buff))) <= 0)
		{
			printf("client exit...\n");
			close(fd);
			pthread_exit((void *)1);
		}
		int i;
		printf("recv date:");
		for(i = 0; i<len; i++)
		{
			printf("%#x ", buff[i]);
		}
		printf("\n");
	}
}


// 初始化服务器套接字地址
int  sock_serv_init(void)
{
	int sockfd ;
	struct sockaddr_in  addr;
	unsigned char  local_ipbuf[20] = {0};
	
	if( !Get_ip_str(local_ipbuf))
	{
		exit(1);
	}
	// 创建socket	
	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERV_PORT);
	addr.sin_addr.s_addr = inet_addr(local_ipbuf);
	// 地址重用
	int reuseaddr = 1;
	Setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr));
	// 地址绑定
	Bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
	// 监听
	Listen(sockfd, LISTENMAXLEN);
	printf("listen success...\n");	
	return sockfd;	
}

// 信号处理函数，屏蔽ctr +c  与 ctrl+\信号结束服务端进程，忽略信号

void sig_handle(int sig)
{
	switch(sig)
	{
		case SIGPIPE:
			printf("SIGPIPE...\n");
		break;
		
		case SIGINT:
			printf("SIGINT...\n");
		break;

		case SIGQUIT:
			printf("SIGQUIT...\n");
		break;
	}
}

void  set_signal(void)
{
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, sig_handle);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, sig_handle);
	signal(SIGPIPE, sig_handle);
}

void clean_0x34_uci_database(unsigned char Index)
{
	char  val_buff[100] = {0};
	val_buff[0] = '0';
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentOutputCurrent", ChargerInfo[(Index)].tab_name) < 0)
		return ;
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectSocket", ChargerInfo[(Index)].tab_name) < 0)
		return ;
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectCurrent", ChargerInfo[(Index)].tab_name) < 0)
		return ;
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.EVLinkID", ChargerInfo[(Index)].tab_name) < 0)
		return ;
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.AccPowerEnd", ChargerInfo[(Index)].tab_name) < 0)
		return ;
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.AccPowerStart", ChargerInfo[(Index)].tab_name) < 0)
		return ;
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargStartTime", ChargerInfo[(Index)].tab_name) < 0)
		return ;
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargEndTime", ChargerInfo[(Index)].tab_name) < 0)
		return ;
	if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerWay", ChargerInfo[(Index)].tab_name) < 0)
		return ;
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

void key_init(unsigned char *key)
{
	char i;
	unsigned char val;
	struct timeval	tpstart;
#ifndef NDEBUG
	printf("key value:");
#endif
	for(i=0; i<16; i++)
	{
		if(gettimeofday(&tpstart, NULL)<0)
		{
#ifndef NDEBUG	
			printf("获取时间失败...\n");
#endif
			i = i-1;
			continue;	
		}
		srand(tpstart.tv_usec);
		*(key+i)= rand()%26 + 97;
//		if( (val == '\'') || (val == '\"') || (val == '\n') || (val == '\r') || (val == '\t') || (val == '\\'))
//		{
//			i = i-1;
//			continue;
//		}
		printf("%d ", *(key+i));
		msleep(5);
	}
	printf("\n");
	return ;
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


void msleep(unsigned long mSec){
    struct timeval tv;
    tv.tv_sec=mSec/1000;
    tv.tv_usec=(mSec%1000)*1000;
    int err;
    do{
       err=select(0,NULL,NULL,NULL,&tv);
    }while(err<0 && errno==EINTR);
}

//程序与后台通信，负责接收抄表，推送，更新，控制命令
 //回应相应的命令

void *pthread_service_send(void *arg)
{
    struct finish_task   *task;
    int i;
    int task_cnt= 0;
    char    val_buff[20];
    debug_msg("pthread of send message is running");
    for ( ; ;)
    {
        pthread_mutex_lock(&finish_task_lock);
        while (finish_head == NULL)
        {
            printf("线程休眠\n");
//            sleep(3);
            pthread_cond_wait(&finish_task_cond, &finish_task_lock);
            printf("线程唤醒...\n");
            task_cnt = 0;
        }
        task_cnt++;
        // 移出节点
        finish_task_remove_head(task = finish_head);
        pthread_mutex_unlock(&finish_task_lock);

        // 格式化数据，发送给服务器
        // 写入luci数据库
        debug_msg("pthread of sending finish deal commands finish");
        debug_msg("task-->info.cmd = %d", task->info.cmd);
        debug_msg("task-->info.cid = %d", task->info.cid);
        debug_msg("task-->way = %d", task->way);
        if (task->way == WEB_WAY)
        {
           for (i = 0; i < charger_manager.present_charger_cnt; i++)
           {
               if (task->info.cid == ChargerInfo[i].CID)
               {
	                 ev_uci_save_action(UCI_SAVE_OPT, true, "1", "chargerinfo.%s.STATUS", ChargerInfo[i].tab_name); 
                    // 来自页面的操作
                    switch (task->info.cmd)
                    {
                        case    WAIT_CMD_CHAOBIAO:
                                sprintf(val_buff, "%ld%c", time(0), '\0');
	                            ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CB_END_TIME", ChargerInfo[i].tab_name); 
	                            sprintf(val_buff, "%d%c", task->info.chargercode, '\0');
                                ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CB_NUM", ChargerInfo[i].tab_name); 
                        break;
                        case    WAIT_CMD_ONE_UPDATE:
                        break;
                        case    WAIT_CMD_CONFIG:
                                sprintf(val_buff, "%ld%c", time(0), '\0');
	                            ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CONF_END_TIME", ChargerInfo[i].tab_name); 
                        break;
                        case    WAIT_CMD_ALL_UPDATE:
                                debug_msg("here");
                                sprintf(val_buff, "%ld%c", time(0), '\0');
	                            ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.FW_END_TIME", ChargerInfo[i].tab_name); 
	                            ev_uci_save_action(UCI_SAVE_OPT, true, "1", "chargerinfo.%s.STATUS", ChargerInfo[i].tab_name); 
                
                         break;
//                        case    WAIT_CMD_ALL_CONFIG:
                            
//                        break;
                    }
               }
           }

        } else if (task->way == SERVER_WAY)
        {
            // 发送给消息队列
        
        }
        free(task); 
    }
}

void * pthread_service_receive(void *arg)
{
    DIR *dir;
    struct  dirent  *ent = NULL;
    int CMD = 0;
    int CID = 0;
    int CID_TMP;
    char    i;
    char    have_cid_flag;
    char    update_cnt;
    char    val_buff[250] = {0};
    char    name[10];
    time_t  tm;
    struct  tm *tim;
    time_t  start_time;
    time_t  end_time;
    char    CMD_TMP;
    char    index;
    time_t  init_time = 1262275200;
    RECV_CMD    cmd;
    // 初始化chargerinfo命令
    sleep(10);
    ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.CMD");
    ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.CID");
    ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.START_TIME");
    ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.END_TIME");
    debug_msg("pthread of receive message is running");
   for ( ; ; )
   {
         update_cnt = 0;
         have_cid_flag = 0;
        msleep(5000); 
#if 1
         if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.CID") < 0) //CMD 
            goto set_zero;
         CID_TMP = atoi(val_buff);
         if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.CMD") < 0) //CMD 
            goto set_zero;
         CMD_TMP = atoi(val_buff);
         if(CMD_TMP == WAIT_CMD_CHAOBIAO) //抄表
         {
//            if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.START_TIME") < 0) //CMD 
//                 goto set_zero;
//             start_time  = atoi(val_buff);
            if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.END_TIME") < 0) //CMD 
                 goto set_zero;
             end_time = atoi(val_buff);
         }
       debug_msg("!CID = %d, !CID_TMP= %d, !CMD=%d, !CMD_TMP=%d\n", CID, CID_TMP, CMD, CMD_TMP); 

        if ( CID == 0 && CMD_TMP <= 0 )
        { 
            CMD = CMD_TMP;
            CID = CID_TMP;
            continue;
        }
        if (CID ==CID_TMP )
        {
            if (CMD == CMD_TMP)
            {
                continue;
            }
        }
        CID = CID_TMP;
        CMD = CMD_TMP;
        debug_msg("CID = %d, CID_TMP= %d, CMD= %d, CMD_TMP=%d\n", CID, CID_TMP, CMD, CMD_TMP); 
        // 控制一下有效的CID
        for (i=0; i< charger_manager.present_charger_cnt; i++)
        {
            if (CMD == WAIT_CMD_ALL_UPDATE || CID == ChargerInfo[i].CID)
            {
                have_cid_flag = 1;
                index = i;
                break;
            }
        }
        if (have_cid_flag != 1)
        {
            // 告诉后台
            log_msg("non match cid");
            sleep(5);
            goto set_zero;
        }
	     
        
        if (CMD == WAIT_CMD_ALL_UPDATE)
        {
               sprintf(val_buff, "%s/%s%c", WORK_DIR, UPDATE_DIR, '\0');
               if ( (dir = opendir(val_buff)) == NULL)
                   continue;
               while ( (ent = readdir(dir)) )
               {
                    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                        continue;
                    strcpy(val_buff, ent->d_name);
                    break;
               }
               closedir(dir);
        } 
      // 封装命令
      cmd.cid = CID;
      cmd.cmd = CMD;
      switch (CMD)
      {
        case    WAIT_CMD_CHAOBIAO:
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.charger%d.CB_END_TIME", index) < 0)
                {
                    cmd.u.chaobiao.start_time =  init_time;
                } else
                {
                    if (atoi(val_buff) != 0)
                    {
                        cmd.u.chaobiao.start_time =  atoi(val_buff);
                    } else
                    {
                        cmd.u.chaobiao.start_time =  init_time;
                    }
                }
                cmd.u.chaobiao.start_time = init_time;

//                cmd.u.chaobiao.start_time =  time(0) - 5*3600*24;//end_time; //start_time;
                cmd.u.chaobiao.end_time = time(0);//end_time;
                debug_msg("pthread of receive command is CHAOBIAO");
        break;
        case    WAIT_CMD_CONFIG:
                sprintf(val_buff, "%08d%c", CID, '\0');
                strcpy(cmd.u.name, val_buff);
                debug_msg("pthread of receive command is CONFIG");
                printf("val_buff:%s\n", cmd.u.name);
        break;
        case    WAIT_CMD_ONE_UPDATE:
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.TABS.chargerversion") < 0) 
                {
                    cmd.version[0] = 1;
                    cmd.version[1] = 6;
                }else
                {
                    cmd.version[0] = atoi(val_buff + 1);
                    cmd.version[1] = atoi(strchr(val_buff, '.')+1);
                    printf("version:%d %d\n", cmd.version[0], cmd.version[1]);
                }
                sprintf(val_buff, "%s%c", "CBMB.bin", '\0');
                strcpy(cmd.u.name, val_buff);
        break;
        case    WAIT_CMD_ALL_UPDATE:
               // if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.TABS.chargerversion") < 0) 
                //{
                    cmd.version[0] = 1;
                    cmd.version[1] = 6;
               // }else
               // {
                    printf("read file = %s\n", val_buff);
                    name[0] = val_buff[6];
                    name[1] = '\0';
                    cmd.version[0] = atoi(name);//atoi(val_buff + 1);
                    //strtok(val_buff, ".");
                    //cmd.version[1] = atoi(strtok(NULL, "."));
                    name[0] = val_buff[8];
                    name[1] = '\0';
                    cmd.version[1] = atoi(name);
                    for (i = 0; i < charger_manager.present_charger_cnt; i++)
                    {
                        if (ChargerInfo[i].free_cnt_flag == 1)
                        {
                            cmd.cid = ChargerInfo[0].CID;
                            break;
                        }
                    }
                    printf("version:%d %d\n", cmd.version[0], cmd.version[1]);
              //  }
                sprintf(val_buff, "%s%c", val_buff, '\0');
                strcpy(cmd.u.name, val_buff);
                debug_msg("pthread of service read all update file = %s\n", ent->d_name);
        break;
        case    WAIT_CMD_ALL_CHAOBIAO:
                cmd.u.chaobiao.start_time = end_time; //start_time;
                cmd.u.chaobiao.end_time = time(0);//end_time;
        break;
      }
      // 加入等待队列
        ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.STATUS", "CLIENT"); //
        sprintf(val_buff, "%d%c", CMD_TMP, '\0');
	    ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CMD", "CLIENT");
        sprintf(val_buff, "%08d%c", CID, '\0');
	    ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CID", "CLIENT");

      wait_task_add(WEB_WAY, cmd);
      debug_msg("pthread of service add task queue success");
//      printf("=============================>加入链表命令操作成功\n");
set_zero:
	  ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.CMD", "SERVER");
       CMD = CMD_TMP = 0; 
#endif
   } //end for
    return NULL;
}

struct load_balance {
    unsigned char   real_time_current; //时实电流
    unsigned char   charging_flag;      // 充电标志
    unsigned char   set_timeout;        // 设置超时
    unsigned char   no_match_flag;
    int             cid;
};
int  bin_packing(unsigned char *data, unsigned char *limit, int len, int surpls)
{
    int i = len;
    int m = 0;
    
    for(i = 0; i < len; i++)
    {
        if (data[i] < limit[i])
            data[i] += 1;
        else
        {
            m++;
            continue;
        }
        if( --surpls == 0)
            return 0;
    }
    if (m == len)
        return (surpls);
    return bin_packing(data, limit, len, surpls);
}
int  bin_packing_err(unsigned char *data, unsigned int limit, int len, int surpls)
{
    int i = len;
    int m = 0;
    
    if (surpls == 0)
        return 0;
    for (i = 0; i < len; i++)
    {
        if (data[i] >limit)
        {
            data[i] -= 1;
        } else
        {
            m++;
            continue;
        }
        if (--surpls == 0)
            return 0;
    }
    if (m == len)
        return surpls;
    return bin_packing_err(data, limit, len, surpls);
}

void *load_balance_pthread(void *arg)
{
      int i;
      unsigned char CNT,NUM, NUM_TMP = 0, have_charger_flag, charger_index, index, sum;
      unsigned char limit[8], current[8], no_change_index[8];
               char  mark_index[8];
               char  surpls;
      unsigned char  wait_time, off_net_cnt;
      struct load_balance balance[8] = {0};
      int cnt = 0;
      char      send_buff[10] = {0};

      sleep(20); 
      debug_msg("pthread of load balance is running");
      for (; ;)
      {
          CNT = charger_manager.present_charger_cnt; // 充电桩个数
          have_charger_flag = 0;
          index = 0;
          off_net_cnt = 0;
          NUM = 0;
	    if(ev_uci_data_get_val(send_buff, 10, "chargerinfo.%s%c%s", "TABS", '.',  "chargernum") >= 0)
        {
            if (atoi(send_buff) > 0)
                charger_manager.total_num = atoi(send_buff);
            printf("max_num = %d\n", charger_manager.total_num);
        }
	    if(ev_uci_data_get_val(send_buff, 10, "chargerinfo.%s%c%s", "TABS", '.',  "maxcurrent") >= 0)
        {
            if (atoi(send_buff) > 0)
                charger_manager.limit_max_current = atoi(send_buff);
            printf("max_current = %d\n", charger_manager.limit_max_current);
        }


          for (i = 0; i < CNT; i++)
          {
                if (ChargerInfo[i].cmd == 0x54) // 有拍卡充电
                {
                    have_charger_flag = 1;
                    charger_index = i;
                }
                if (ChargerInfo[i].flag == 1)
                {
                    balance[i].charging_flag = 1;
                    NUM++;

                } else
                {
                    balance[i].charging_flag = 0;      // balance 对应电桩序列,复制到局部变量，防止被改变
                }
		if(ChargerInfo[i].free_cnt_flag == 1)
			{
				ChargerInfo[i].free_cnt++;
				off_net_cnt++;
				debug_msg("%d :%d", i, ChargerInfo[i].free_cnt);
				if (ChargerInfo[i].free_cnt > 5)
					ChargerInfo[i].free_cnt_flag = 0;
			}
                if (ChargerInfo[i].free_cnt_flag == 0)
                {
			debug_msg("Charger %d disconnected:", ChargerInfo[i].CID);
			ev_uci_save_action(UCI_SAVE_OPT, true, "46", "chargerinfo.%s.PresentMode", ChargerInfo[i].tab_name);
                }
        }
	    charger_manager.present_off_net_cnt = charger_manager.total_num - off_net_cnt;
//        debug_msg("pthread of load balance: off net cnt = %d", charger_manager.present_off_net_cnt);
        // 向串口发送灯板控制命令
        if(NUM <= 5)
        {
            send_buff[0] = 0x53;
            send_buff[4] = 0x45;
            send_buff[1] = POWER_BAR_PWN_3S;  
            send_buff[2] = POWER_BAR_GREEN;  
            send_buff[3] = (6-NUM) + 0x30;  
            send_buff[5] = '\0';
            cmd_frun("echo %s > /dev/ttyUSB0", send_buff);
//              power_bar_ctrl_send(charger_manager.have_powerbar_serial_fd, POWER_BAR_PWN_3S, POWER_BAR_GREEN, 6 -NUM);
        }
        else if(NUM >= CNT)
        {
            send_buff[0] = 0x53;
            send_buff[4] = 0x45;
            send_buff[1] = POWER_BAR_PWN_3S;  
            send_buff[2] = POWER_BAR_RED;  
            send_buff[3] = 6 + 0x30;  
            send_buff[5] = '\0';
            cmd_frun("echo %s > /dev/ttyUSB0", send_buff);
//              power_bar_ctrl_send(charger_manager.have_powerbar_serial_fd, POWER_BAR_PWN_3S, POWER_BAR_RED, 6);
        }
        else
        {
            send_buff[0] = 0x53;
            send_buff[4] = 0x45;
            send_buff[1] = POWER_BAR_PWN_3S;  
            send_buff[2] = POWER_BAR_GREEN;  
            send_buff[3] = 1 + 0x30;  
            send_buff[5] = '\0';
            cmd_frun("echo %s > /dev/ttyUSB0", send_buff);
//                power_bar_ctrl_send(charger_manager.have_powerbar_serial_fd, POWER_BAR_PWN_3S, POWER_BAR_GREEN, 1);
        }
        debug_msg("load_balance============>charger_cnt:%d, charing_cnt:%d, net_off_cnt:%d \n", CNT, NUM, charger_manager.present_off_net_cnt);
	msleep(10000);
#if 0
        // 不进行分配的条件
          if (have_charger_flag == 0 && NUM <= 0 || CNT == 0)  // (have_charger_flag == 0 && NUM = NUM_TMP)
            {
//                if (NUM == NUM_TMP)
//                {
//                    NUM_TMP = NUM;
                    sleep(10);
                    continue;
//                }
            }
           printf("*********************************\n");
           printf("**       开始进行电流分配     ***\n");
           printf("*********************************\n");
           NUM_TMP = NUM;
          //分配充电电流, 7A
          debug_msg("have_charger_flag = %d\n", have_charger_flag );
          if (have_charger_flag == 1)
          {
                index = 0;
                for (i = 0; i < CNT; i++)
                {
                    if (balance[i].charging_flag == 1)
                    {
                        ChargerInfo[i].real_current = 7;
                        index++;
                    }
                 }
                // 等待 1min, 提前相等直接跳出
                cnt = 0;
                wait_time = 20;
                char no_cnt = 0;
                while (wait_time--)
                {
                    memset(no_change_index, 0, sizeof(no_change_index));
                    debug_msg("正在等待.....\n");
                    cnt = 0;
                    for ( i = 0; i < CNT; i++)
                    {
                        if (balance[i].charging_flag == 1)
                        {
                            if (ChargerInfo[i].real_time_current == 7)
                               cnt++;
                            else
                                no_change_index[i] = 1;
                        }
                    }
                    if (index == cnt)
                        break;
                    sleep(1);
                }
        
                // 判断有没有可用电流
                sum = 0;
                for (i = 0; i < CNT; i++)
                {
                    if (balance[i].charging_flag == 1)
                    {
                        sum += ChargerInfo[i].real_time_current;
                    }
                }
                surpls = charger_manager.limit_max_current - sum - 7 * charger_manager.present_off_net_cnt;
                debug_msg("$$$$$$$$$$$$======>surpls:%d\n",surpls);
                if (surpls >= 7)
                {
                    ChargerInfo[charger_index].real_current = 7;
                    sleep (2);
                    continue;
                } else
                {
                    sleep (2);
                    continue;
                }
                // 跳转
                goto READ;
          } // end if 
READ:
        // 重读电桩实时电流
        sleep (10);
        index = 0;
        sum = 0;
        char cur = 0;
        memset(mark_index, -1, sizeof(mark_index));
        for (i = 0; i < CNT; i++)
        {
            if (balance[i].charging_flag == 1)
            {
                cur = ChargerInfo[i].real_time_current;
                if (no_change_index[i] !=  1)
                {
                    current[index] = cur; //ChargerInfo[i].real_time_current;
                    limit[index] = ChargerInfo[i].model;
               // 记录其改变了的下标
                    mark_index[i] = i;
                    index++;
                }
                sum += cur;  //current[index];
            }
        }
        // 计算剩余
        for (i = 0; i < index; i++)
        {
            printf("$$$$$$$$$$$$$$$$$$======>limit:%d\n", limit[i]);
        }
        printf("$$$$$$$$$$$$======>surpls:%d\n",surpls);
//        surpls = charger_manager.limit_max_current - sum;
        surpls = charger_manager.limit_max_current - sum - 7 * charger_manager.present_off_net_cnt;
        if (surpls <= 0)
             bin_packing_err(current, 7, index, 0 - surpls);
         else
            bin_packing(current, limit, index, surpls);
        printf("$$$$$$$$$$$$======>surpls:%d\n",surpls);
        for (i = 0; i < index; i++)
        {
            printf("$$$$$$$$$$$$$$$$$$======>%d\n", current[i]);
        }
        index = 0;
        for (i = 0; i < 8; i++)
        {
            printf("%d ", no_change_index[i]);
        }
        printf("\n");
        for ( i = 0; i < CNT; i++)
        {
            if (balance[i].charging_flag == 1)
            {
                if (no_change_index[i] == 1)
                    ChargerInfo[i].real_current = ChargerInfo[i].real_time_current;
                else
                {
                    if (mark_index[i] == i) // 已经改成7A
                        ChargerInfo[i].real_current = current[index++];
                }
            }
        }
        continue;
#endif
      } 
    return (void *)NULL;
}

//取出数据库配置信息，然后存入内存中
void  charger_info_init(int select)
{
	//查询数据库，获取表名数据
	unsigned  char *tab_name = NULL;
	unsigned char *tab_tmp = NULL;
	unsigned char *str;
	unsigned char name[10][10] = {0}, info[50], i = 0, j=0, len, charg_cnt = 0;
    int data, ii; 
    unsigned char tmp[2];

    if (select)
    {
	    // 存取充电记录变量
        memset(info, 0, sizeof(info));
        strcpy(info, "V1.01");
        ev_uci_save_action(UCI_SAVE_OPT, true, info, "chargerinfo.%s.chargerversion", "TABS"); 	
        sprintf(info, "%d%c", charger_manager.limit_max_current, '\0');
        ev_uci_save_action(UCI_SAVE_OPT, true, info, "chargerinfo.%s.maxcurrent", "TABS"); 	
        sprintf(info, "%d%c", charger_manager.total_num, '\0');
        ev_uci_save_action(UCI_SAVE_OPT, true, info, "chargerinfo.%s.chargernum", "TABS"); 	
        return ;
    }
    printf("uci  init ... \n");
	if( (tab_tmp = tab_name = find_uci_tables(TAB_POS)) == NULL)
		return ;
	while(*tab_name)
	{
		if(*tab_name == ',')
		{
	        printf("charger name[%d] = %s\n",i,  name[i]);
			i++;
			j = 0;
			tab_name++;
			continue;
		}
		name[i][j++] = *tab_name++;		
	}
	printf("charger name[%d] = %s\n",i,  name[i]);
    printf("i+1 = %d\n", i+1);
	for(j =0; j<i+1; j++)
	{
		if(ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.',  "CID") < 0){ //获取CID
            continue;
        }
		ChargerInfo[j].CID = atoi(info);
#ifndef NDEBUG
		printf("数据库取出的CID为------------------>%d\n", ChargerInfo[j].CID);
#endif	
        bzero(info, sizeof(info));
        ev_uci_data_get_val(info, 40, "chargerinfo.%s%c%s", name[j], '.', "PresentMode"); //
        debug_msg("presentmode = %d", atoi(info)); 
        if (atoi(info) == CHARGER_CHARGING)
        {
            debug_msg("服务正在重启，有电桩正在充电");
            bzero(info, sizeof(info));
            if (ev_uci_data_get_val(info, 40, "chargerinfo.%s%c%s", name[j], '.', "privateID") < 0) //
                goto ret;
            debug_msg("privaid:%s", info); 
            for (ii = 0; ii < 16; ii++)
            {
                data = 0;
                tmp[0] = info[2 * ii];
                tmp[1] = info[2 * ii + 1];
                if (tmp[0] <= '9')
                {
                    data += (16*(tmp[0] - '0'));
                } else
                {
                    data += (16*(tmp[0] - 'a' + 10));
                }
               if (tmp[1] >= 'a') 
               {
                    data +=  (tmp[1] - 'a' + 10);
               } else
               {
                    data += (tmp[1] - '0');
               }
               debug_msg("%x ", data);
                ChargerInfo[j].ev_linkid[ii] = data;     
            }
        }
ret:
        bzero(info, strlen(info));
		if(ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.', "Model") < 0) // IP
		    exit(1);
		if(strncmp(info, "EVG-32N", 7) == 0)
		{
			ChargerInfo[j].model = EVG_32N;
		}
		else
		{
			ChargerInfo[j].model = EVG_16N;	
		}
		bzero(info, strlen(info));
		ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.', "IP"); // IP
		//ChargerInfo[j].IP = inet_addr(info);
		unsigned char* p = info;
		char cnt;
		for(cnt = 0; cnt < 4; cnt++)
		{
			if(cnt == 0){
				str = strtok(p, ".");
				if(str) 	ChargerInfo[j].IP[cnt] = atoi(str);
				continue;
			}
			str = strtok(NULL, ".");
			if(str)
				ChargerInfo[j].IP[cnt] = atoi(str);
		}	
#ifndef NDEBUG
		printf("数据库取出的IP为------------------>%d.%d.%d.%d\n", ChargerInfo[j].IP[0], ChargerInfo[j].IP[1], ChargerInfo[j].IP[2], ChargerInfo[j].IP[3]);
#endif	
		bzero(info, strlen(info));
		// 赋值key值
		if(ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.', "KEYB") < 0)
		    exit(1);
        int c;
		for(c=0; c<16; c++)
			printf("%d ", info[c]);
		printf("\n");
		strncpy(ChargerInfo[j].KEYB, info, 16);
		if(ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.', "MAC") < 0)
		    exit(1);
        strncpy(ChargerInfo[j].MAC, info, 17);
		strcpy(ChargerInfo[j].tab_name, name[j]);
        ChargerInfo[j].real_current = 7; // 7A
		charg_cnt++;
		charger_manager.present_charger_cnt++;	// 全局数组，用于在内存中记录充电桩的个数,所有线程共享	
#ifndef NDEBUG
		printf("当前从数据库取出充电桩个数为--------------->%ld\n", charger_manager.present_charger_cnt);
#endif
	}
//	memcpy(Table_Name, tab_tmp, strlen(tab_tmp));	//赋值表名到全局数组
	free(tab_tmp);
	
    ev_uci_data_get_val(info, 20, "chargerinfo.TABS.maxcurrent");
    charger_manager.limit_max_current = atoi(info);
    ev_uci_data_get_val(info, 20, "chargerinfo.TABS.chargernum");
    charger_manager.total_num = atoi(info);

    for (i = 0 ; i < charger_manager.present_charger_cnt; i++)
    {
        printf("%s  ", ChargerInfo[i].tab_name);
    }
    for (i = 0 ; i < charger_manager.present_charger_cnt; i++)
    {
        printf("read mac:%s\n", ChargerInfo[i].MAC);
    }
    return ;
}

void print_PresentMode(unsigned char pmode)
{
	switch(pmode)
	{
		case CHARGER_READY:
			printf("当前模式为------>就绪状态...\n");
		break;
		case CHARGER_RESERVED:
			printf("当前模式为------>预约状态...\n");
		break;
		case CHARGER_PRE_CHARGING:
			printf("当前模式为------>预充电状态...\n");
		break;
		case CHARGER_CHARGING_IN_QUEUE:
			printf("当前模式为------>充电序列中...\n");
		break;
		case CHARGER_CHARGING:
			printf("当前模式为------>正在充电中...\n");
		break;
		case CHARGER_CHARGING_COMPLETE:
			printf("当前模式为------>充电完成...\n");
		break;
		case CHARGER_OUT_OF_SERVICE:
			printf("当前模式为------>结束服务...\n");
		break;
		case CHARGER_CHARGING_COMPLETE_LOCK:
			printf("当前模式为------>充电完成，还没解锁...\n");
		break;
		case CHARGER_BOOT_UP:
		break;
		case CHARGER_DEBUGGING:
		break;
		case CHARGER_ESTOP:
			printf("当前模式为------>ESTOP...\n");
		break;
	}
}

void print_SubMode(unsigned short submode)
{
	int i = 1;
	printf("submode=%#x\n", submode);
	while(i <= 0xFFFF)
	{
		switch(submode & i)
		{
			case SM_Already_Plug_Line:
				printf("submode为-------->已插线...\n");
			break;
			case SM_Is_Charging:
				printf("submode为-------->正在充电...\n");
			break; 
			case SM_Charge_Complete:
				printf("submode为-------->充电完成...\n");
			break;
			case SM_Have_Parking:
				printf("submode为-------->有泊车...\n");
			break;
			case SM_Connected_Join:
				printf("submode为-------->连接伺服...\n");
			break;
			case SM_13A_Plug:
				printf("submode为-------->13A已连接...\n");
			break;
			case SM_20A_Plug:
				printf("submode为-------->20A已连接...\n");
			break;
			case SM_32A_Plug:
				printf("submode为-------->32A已连接...\n");
			break;
			case SM_63A_Plug:
				printf("submode为-------->63A已连接...\n");
			break;
			case SM_16A_Plug:
				printf("submode为-------->16A已连接...\n");
			break;
		}

		i <<= 1;
	}
}

void print_SelectSocket(unsigned char socket)
{
	switch(socket)
	{
		case SS_BS1363:
			printf("socket为---------->SS_BS1363\n");
		break;
		case SS_SAEJ1772:
			printf("socket为---------->SS_SAEJ1772\n");
		break;
		case SS_IEC62196:
			printf("socket为---------->SS_IEC62106\n");
		break;
		case SS_GB20234:
			printf("socket为---------->SS_GB20234\n");
		break;
		case SS_IEC62196_3phase:
			printf("socket为---------->SS_IEC62106(3phase)\n");
		break;
		case SS_GB20234_3phase:
			printf("socket为---------->SS_GB20234(3phase)\n");
		break;
		case SS_CHAde_Mo:
			printf("socket为---------->SS_CHAde_Mo\n");
		break;
		case SS_IEC_Combo:
			printf("socket为---------->SS_IEC_Combo\n");
		break;
		case SS_GB_QUICK_CHARGER:
			printf("socket为---------->SS_GB_QUICK_CHARGER\n");
		break;

	}
}

