

//基于TCP的EV充电桩与EV_Router通信 服务器端
#include <libev/msgqueue.h>
#include <libev/uci.h>
#include "include/err.h"
#include "include/CRC.h"
#include "include/AES.h"
#include "include/list.h"
#include "include/ev_lib.h"
#include "include/ev_hash.h"
#include "include/serv_config.h"
#include "include/ev_command_file.h"

//pthread_mutex_t		clifd_mutex = PTHREAD_MUTEX_INITIALIZER;	// 线程互斥锁初始化
//pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;  // 线程条件变量初始化
pthread_mutex_t		new_task_mutex = PTHREAD_MUTEX_INITIALIZER;	        // 线程互斥锁初始化
//pthread_cond_t		serv_cond = PTHREAD_COND_INITIALIZER;		// 线程条件变量初始化
//pthread_rwlock_t	charger_rwlock;


sigset_t        mask;

// 函数声明
void thread_make(ev_int i);
int  sock_serv_init(void);
void msleep(unsigned long mSec);
void print_PresentMode(unsigned char pmode);
void print_SubMode(unsigned short submode);
void print_SelectSocket(unsigned char socket);
void  charger_info_init(int select);
void *pthread_listen_program(void * arg);
void *pthread_service_send(void *arg);
void *pthread_service_receive(void *arg);
void *sigle_pthread(void *arg);
CHARGER_INFO_TABLE *look_for_charger_from_array(int cid);
struct NewTask *parse_server_command(const char *command);
int    check_have_one_handle(CHARGER_INFO_TABLE *charger, struct NewTask *task, \
        void (*commit_wrong)(int, const CHARGER_INFO_TABLE *, const char *, int));


void  dir_init(void)
{
    char    name[100];
    mkdir(WORK_DIR, 0711);
    
    if (access(WORK_DIR, F_OK) != 0)
          exit(1);
    sprintf(name, "%s/%s", WORK_DIR, CHAOBIAO_DIR);
    //创建抄表目录
    mkdir(name, 0711);

    sprintf(name, "%s/%s", WORK_DIR, CONFIG_DIR);
    mkdir(name, 0711);

    sprintf(name, "%s/%s", WORK_DIR, UPDATE_DIR);
    mkdir(name, 0711);

    sprintf(name, "%s/%s", WORK_DIR, RECORD_DIR);
    mkdir(name, 0711);

    sprintf(name, "%s/%s", WORK_DIR, EXCEPTION_DIR);
    mkdir(name, 0711);

    sprintf(name, "%s/%s", WORK_DIR, LOG_DIR);
    mkdir(name, 0777);
    
    mkdir(ROUTER_LOG_DIR, 0777);
}

void sig_handler(int sig)
{
	printf("定时器被唤醒....%d\n", sig);
}

void uci_database_init(void)
{
	if (access(CHARG_FILE,  F_OK) != 0)
	{
                file_trunc_to_zero(CHARG_FILE); //CHARG_FILE);	//创建/etc/config/chargerinfo文件
//                ev_uci_add_named_sec("chargerinfo.%s=tab", "TABS");
//		ev_uci_add_named_sec("chargerinfo.%s=rod", "Record");
                ev_uci_add_named_sec("chargerinfo.%s=issued", "SERVER");
                ev_uci_add_named_sec("chargerinfo.%s=respond", "CLIENT");
                ev_uci_add_named_sec("chargerinfo.%s=chargerinfo", "chargers");
		charger_info_init(1); // 初始化数据库数据 
	} else 	
		charger_info_init(0); //数据库中读入信息，写入内存数组中，如果没有，也直接运行。(CID, IP, KEY)
}
void signal_init(void)
{
	struct  sigaction   sa;
        
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = SIG_IGN;
        if (sigaction(SIGPIPE, &sa, NULL) < 0)
                err_sys("sigction failed");
//      if (sigaction(SIGINT, &sa, NULL) < 0)
//              err_sys("sigction failed");
        if (sigaction(SIGQUIT, &sa, NULL) < 0)
                err_sys("sigction failed");
        if (sigaction(SIGALRM, &sa, NULL) < 0)
                err_sys("sigction failed");
        if (sigaction(SIGCHLD, &sa, NULL) < 0)
                err_sys("sigction failed");
#if 0
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGALRM);
    if ( (err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
        err_sys("pthread_sigmask failed");
#endif 

}

int listenfd;
int pipefd;
int opt_type;

#define M_ARENA_MAX     -8
int main(int argc , char * argv[])
{
	//服务器初始化
	// 设置信号处理
	//5.响应

	int                 err;
	pthread_t           tid;
        
        opt_type = load_command_line(argc, argv);
        command_init_for_run(opt_type, 
                                       OPTION_HELP | OPTION_VERSION | 
                                       OPTION_DAEMON | OPTION_START | 
                                       OPTION_STOP | OPTION_RESTART | 
                                       OPTION_DEBUG | OPTION_PROCESSLOCK);
        // controller初始化
        protocal_controller_init();
        // 散列表初始化
        global_cid_hash = hash_alloc(HASH_BUCKETS, hash_func); 
        // 目录初始化
        dir_init(); 
        // 信号初始化
        signal_init();
        // 数据库初始化
        uci_database_init();
        // 长连接下管道初始化
        unlink(PIPEFILE);
        if (mkfifo(PIPEFILE, 0777) < 0)
              err_sys("mkfifo %s failed", PIPEFILE);
        // 服务器地址初始化
        listenfd = sock_serv_init();
//      	command_init_for_run(opt_type, OPTION_LOADBALANCE);
//    if ( (charger_manager.have_powerbar_serial_fd  = UART_Open(POWER_BAR_SERIAL)) < 0)
//        err_sys("open power bar serial error");
//    if ( UART_Init(charger_manager.have_powerbar_serial_fd, 9600, 0, 8, 1, 'N') < 0)
//        err_sys("init power bar serial error error");
	// 保护充电信息的读写锁，初始化
//	if ((err = pthread_rwlock_init(&charger_rwlock, NULL)) < 0)
//		errno = err, err_sys("pthread_rwlock_init error");
       if( (err = pthread_create(&tid, NULL, &pthread_service_send, (void *)0)) < 0)
		errno = err, err_sys("pthread_create error");
        if( (err = pthread_create(&tid, NULL, &pthread_service_receive, (void *)0)) < 0)
		errno = err, err_sys("pthread_create error");
    
//      if ((err = pthread_create(&tid, NULL, signal_pthread, NULL)) < 0)
//               errno = err, err_sys("pthread_create failed");
        // 服务程序     
        command_init_for_run(opt_type, OPTION_RUNNING);
        
	return EXIT_SUCCESS;
}

void send_cmd_to_chargers(struct PipeTask *task, CHARGER_INFO_TABLE *charger)
{
        int cmd_type;
        
        if (charger == NULL)
             return ;
         // 查找电桩
        if (task->cmd == WAIT_CMD_WAIT_FOR_CHARGE_REQ)
        {
            if ( (charger->system_message = task->value) == 0x01)
                charger->target_mode = CHARGER_CHARGING;
            else if ( (charger->charger_type == 1 || charger->charger_type == 3) && task->value == SYM_OFF_NET)
            {
                charger->target_mode = CHARGER_CHARGING;
                charger->system_message = 0x01;
            }
            else
                 charger->target_mode = CHARGER_READY;
             cmd_type = CHARGER_CMD_AUTH_R;
        }
        else if (task->cmd == WAIT_CMD_CONFIG)
            cmd_type = CHARGER_CMD_CONFIG_R;
        else if (task->cmd == WAIT_CMD_YUYUE)
            cmd_type = CHARGER_CMD_YUYUE_R;
        else if (task->cmd == WAIT_CMD_START_CHARGE)
            cmd_type = CHARGER_CMD_START_CHARGE_R;
        else if (task->cmd == WAIT_CMD_STOP_CHARGE)
            cmd_type = CHARGER_CMD_STOP_CHARGE_R;
        else if (task->cmd == WAIT_CMD_ONE_UPDATE)
            cmd_type = CHARGER_CMD_START_UPDATE_R;
        else if (task->cmd == WAIT_CMD_CTRL)
            cmd_type = CHARGER_CMD_CONTROL_R;
        else if (task->cmd == WAIT_CMD_CHAOBIAO)
            cmd_type = CHARGER_CMD_RECORD_R;
        start_service(charger->sockfd, NULL, cmd_type, charger->CID);
//        do_serv_run(charger->sockfd, cmd, bf, charger, error_handler);
}
typedef struct String {
        char *value;
        int  num;
}string;

void evdoit_l_serv(void)
{
        string          buff = {0};
        int             maxfd, i, connfd, sockfd, n, pipefd, equ_sock = -1;
        int             nready, client[FD_SETSIZE];
        fd_set          rset, allset;
        struct          PipeTask   pipe_task;
        CHARGER_INFO_TABLE *charger;
        
        if ( (pipefd = open(PIPEFILE, O_RDONLY, 0400)) < 0)
                err_sys("open %s failed", PIPEFILE);
        
        if ( (buff.value  = (char * )calloc(MAX_LEN, 1)) == NULL)
                goto over;
        
        maxfd = listenfd > pipefd ? listenfd : pipefd;
        for ( i = 0; i < FD_SETSIZE; i++)
                client[i] = -1;
        FD_ZERO(&allset);
        FD_SET(listenfd, &allset);
        FD_SET(pipefd,   &allset);
        printf("evdoit_l_serv pipefd=%d\n",  pipefd);
        for ( ; ; )
        {
                rset = allset;
                nready = select(maxfd+1, &rset, NULL, NULL, NULL);

                if (FD_ISSET(listenfd, &rset))
                {
		        if ((connfd = accept(listenfd, NULL, NULL)) < 0 ){
			        if(errno == EINTR || errno == ECONNABORTED)
				        continue;
		        	else
                                        err_sys("accept error");
		        }
                        for(i = 0; i < FD_SETSIZE; i++)
                        {
                                if (client[i] < 0){
                                        client[i] = connfd;
                                        break;
                                }
                        }
                        if (i == FD_SETSIZE)
                            err_quit("too many clients");

                        FD_SET(connfd, &allset);
                        if (connfd > maxfd)
                            maxfd = connfd;
                        if (--nready <= 0)
                            continue;
                }
#if 1
                if (FD_ISSET(pipefd, &rset))
                {
                        // 读取管道
                        if ( (n = read(pipefd, &pipe_task, sizeof(struct PipeTask))) < 0){
                             exit(EXIT_FAILURE); 
                        }
                        printf("pipe receive data ... \n");
                        if (n != sizeof(struct PipeTask))
                            continue;
                        // 发送命令
                         charger = look_for_charger_from_array(pipe_task.cid);
                         // fcntl
//                         if (recv(charger->sockfd, buff.value, MAX_LEN, O_NONBLOCK | MSG_PEEK) > 0)
//                         {
//                                printf("套接字缓冲区有数据 ...\n");
//                                read(charger->sockfd, buff.value, MAX_LEN);
//                                FD_CLR(charger->sockfd, &rset);
//                                printf("%s\n", buff.value);
//                         }
//                          else 
//                                printf("套接字缓冲区没有数据 ...\n");
//                         equ_sock = charger->sockfd;
                         send_cmd_to_chargers(&pipe_task, charger);
                         FD_CLR(pipefd, &rset);
                         continue;
                }
#endif
                for ( i = 0; i < FD_SETSIZE; i++)
                {
                        if ((sockfd = client[i]) < 0)
                            continue;
                        if (FD_ISSET(sockfd, &rset))
                        {
                                printf("sockfd:%d\n", sockfd);
                                if ( (n = read(sockfd, buff.value, MAX_LEN - 1)) <= 0)
                                {
                                        close(sockfd);
                                        FD_CLR(sockfd, &allset);
                                        client[i] = -1;
                                } else
                                {
                                        buff.num = n;
                                        buff.value[n] = '\0';
//                                        if (equ_sock != sockfd)
                                                start_service(sockfd, buff.value, 0, 0);
//                                        else
//                                                equ_sock = -1;
                                }

                                if (--nready <= 0)
                                    break;
                        }
                }
        } // end for
over:
        return ;
}

CHARGER_INFO_TABLE *look_for_charger_from_array(int cid)
{
    void *charger;

    charger = hash_lookup_entry(global_cid_hash, &cid, sizeof(cid));
    
    return (CHARGER_INFO_TABLE *)charger;
}

#include <netinet/tcp.h>

// 初始化服务器套接字地址
int  sock_serv_init(void)
{
	int sockfd ;
	struct sockaddr_in  addr;
	char  local_ipbuf[20] = {0};
	
	if( !get_ip_str(local_ipbuf))
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
        int keepalive = 1;
        int keepidle = 60; // 60s进行探测
        int keepinterval = 5; //探测时发包的时间间隔
        int keepcount = 3;    //探测时尝试的次数，成功后就不再发送
        Setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&keepalive, sizeof(keepalive));  // keepalive
        Setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (const void *)&keepidle, sizeof(keepidle));
        Setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (const void *)&keepinterval, sizeof(keepinterval));
        Setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT,  (const void *)&keepcount, sizeof(keepcount));
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


void msleep(unsigned long mSec){
    struct timeval tv;
    int err;

    tv.tv_sec=mSec/1000;
    tv.tv_usec=(mSec%1000)*1000;
    do{
       err=select(0,NULL,NULL,NULL,&tv);
    }while(err<0 && errno==EINTR);

}

void *pthread_send_to_backstage(void *arg)
{
#if 0
        struct finish_task *task;
        struct join_new_task  new_task;
        int pipefd;
        char  val_buff[200];
        CHARGER_INFO_TABLE *charger;


        task = (struct finish_task *)arg;
        charger = look_for_charger_from_array(task->cid);
        pthread_detach(pthread_self());
        
         // 发送给消息队列i
        if ( task->cmd == WAIT_CMD_CHARGER_API) // 开始充电
        {
               if ( (pipefd = open(PIPEFILE, O_WRONLY, 0200)) < 0){
                            free(task->str);
                            return NULL; 
                }
             printf("string:%s\n", task->str);
             free(task->str);
             char *sptr; // = check_send_status(val_buff);
             if (!sptr)
                     new_task.value = SYM_OFF_NET;
              else
                      new_task.value = sptr[8] - 48;
                
              new_task.cid = task->cid;
//              new_task.cmd = WAIT_CMD_CHARGE_REQ;
              write(pipefd, &new_task, sizeof(struct join_new_task));
              close(pipefd);
                    
          } else if (task->cmd == WAIT_CMD_UPDATE_FAST_API)
          {
                cmd_frun("dashboard update_fast"); 
          } else if (task->cmd == WAIT_CMD_STOP_STATE_API)
            {
                sprintf(val_buff, "%s%s", "/ChargerState/stopState?", task->str);
                printf("string:%s\n", task->str);
               // cmd_frun("dashboard url_post %s %s", sys_checkin_url(), val_buff);
                 free(task->str);
            } else if(task->cmd == WAIT_CMD_PUSH_MESSAGE_API)
            {
                sprintf(val_buff, "%s%s", "/PushMessage/pushMessage?", task->str);
                printf("string:%s\n", task->str);
                //cmd_frun("dashboard url_pos %s %s", sys_checkin_url(), val_buff);
                 free(task->str);
            } else if (task->cmd == WAIT_CMD_CHANGE_STATUS_API)
            {
                sprintf(val_buff, "%s%s", "/ChargerState/changesStatus?", task->str);
                printf("string:%s\n", task->str);
                //cmd_frun("dashboard url_post %s %s", sys_checkin_url(), val_buff);
                 free(task->str);
            }else   if (task->cmd == WAIT_CMD_CHAOBIAO_API)
             {
                                if (task->way == SERVER_WAY)
                                        sprintf(val_buff, "%s/%s/%08d_server", WORK_DIR, CHAOBIAO_DIR, task->cid);
                                else
                                        sprintf(val_buff, "%s/%s/%08d_web", WORK_DIR, CHAOBIAO_DIR, task->cid);
                                printf("file_name:%s\n", val_buff);
                                charger->server_send_status = MSG_STATE_NULL;
                                cmd_frun("dashboard post_file %s", val_buff);
            }
            else if (task->cmd == WAIT_CMD_RESERVED_API)
            {
                //cmd_frun("dashboard url_post %s %s", sys_checkin_url(), val_buff);
                        
            }
       free(task); 
#endif
       return NULL; 
} //end pthread_send_to_backstage

void show_info(int errcode)
{
        switch (errcode)
        {
                case MSG_STATE_CONFIG:
                        printf("无法进行操作，上次命令正在处于配送配置状态 ...\n"); 
                break;
                case MSG_STATE_UPDATE:
                        printf("无法进行操作，上次命令正在处于软件更新状态 ...\n"); 
                break;
                case MSG_STATE_CHAOBIAO:
                        printf("无法进行操作，上次命令正在处于抄表状态 ...\n"); 
                break;
                case MSG_STATE_YUYUE:
                        printf("无法进行操作，上次命令正在处于预约状态 ...\n"); 
                break;
                case MSG_STATE_START_CHARGE:
                        printf("无法进行操作，上次命令正在处于开始充电状态 ...\n"); 
                break;
                case MSG_STATE_STOP_CHARGE:
                        printf("无法进行操作，上次命令正在处于停止充电状态 ...\n"); 
                break;
                case MSG_STATE_CONTROL:
                        printf("无法进行操作，上次命令正在处于控制状态 ...\n"); 
                break;

                // 电桩状态
                case CHARGER_STATE_CONFIG:
                        printf("无法进行操作，电桩正在处于控制状态 ...\n"); 
                break;
                case CHARGER_STATE_UPDATE:
                        printf("无法进行操作，电桩正在处于更新状态 ...\n"); 
                break;
                case CHARGER_STATE_CHAOBIAO:
                        printf("无法进行操作，电桩正在处于抄表状态 ...\n"); 
                break;
                case CHARGER_STATE_OFF_NET:
                        printf("无法进行操作，电桩正在处于断网状态 ...\n"); 
                break;
                case CHARGER_READY:
                        printf("无法进行操作，电桩正在处于就绪状态 ...\n"); 
                break;
                case CHARGER_RESERVED:
                        printf("无法进行操作，电桩正在处于预约状态 ...\n"); 
                break;
                case CHARGER_CHARGING:
                        printf("无法进行操作，电桩正在处于充电状态 ...\n"); 
                break;
                case CHARGER_CHARGING_COMPLETE:
                        printf("无法进行操作，电桩正在处于充电完成状态 ...\n"); 
                break;
                case CHARGER_OUT_OF_SERVICE:
                        printf("无法进行操作，电桩正在处于暂停服务状态 ...\n"); 
                break;
                case CHARGER_CHARGING_COMPLETE_LOCK:
                        printf("无法进行操作，电桩正在处于充电完成还未解锁状态 ...\n"); 
                break;
                case CHARGER_ESTOP:
                        printf("无法进行操作，电桩正在处于紧急制已按状态 ...\n"); 
                break;
                case FILE_NO_EXIST:
                        printf("无法进行操作，文件不存在 ...\n");
                break;
                case FILE_SIZE_ZERO:
                        printf("无法进行操作，文件大小为0 ...\n");
                break;
                case CHARGER_NO_EXIST:
                        printf("无法进行操作，电桩不存在 ...\n");
                break;
                case SERVER_DEAL_ERROR:
                        printf("无法进行操作，路由处理失败 ...\n");
                break;
                default :
                        printf("无法进行操作，未知状态 ...\n");
                break;
                        
       }
}

void *pthread_service_send(void *arg)
{
    struct finish_task   *dash_info;
    int i;
    char   val_buff[200];
    CHARGER_INFO_TABLE *charger;

    debug_msg("pthread of send message is running");
    for ( ; ;)
    {
        pthread_mutex_lock(&finish_task_lock);
        while (finish_head == NULL)
            pthread_cond_wait(&finish_task_cond, &finish_task_lock);
        // 移出节点
        finish_task_remove_head(dash_info = finish_head);
        pthread_mutex_unlock(&finish_task_lock);

        // 格式化数据，发送给服务器
        // 写入luci数据库
        charger = dash_info->charger;
        debug_msg("pthread of sending finish deal commands finish");
        printf("dash->cmd = %d, cid = %d,  errcode = %d str:%s\n", 
                dash_info->cmd, (charger ? charger->CID : -1), dash_info->errcode, dash_info->str);

       switch (dash_info->cmd)
       {
                case  DASH_BOARD_INFO:
                        cmd_frun("dashboard url_post %s %s", sys_checkin_url(), dash_info->str);
                        free(dash_info->str);
                break;
                case  DASH_FAST_UPDATE:
                        cmd_frun("dashboard update_fast");
                break;
                case  DASH_FINISH_INFO:
                        printf("当前进度完成 ...\n");
                        if (charger->message->new_task.cmd == WAIT_CMD_CHAOBIAO)
                        {
                                if (charger->message->new_task.way == WEB_WAY)
                                        cmd_frun("dashboard post_file %s/%s/%08d_web", WORK_DIR, CHAOBIAO_DIR, charger->CID);
                                 else 
                                        cmd_frun("dashboard post_file %s/%s/%08d_server", WORK_DIR, CHAOBIAO_DIR, charger->CID);
                        } 
                        pthread_mutex_lock(&new_task_mutex);
                        // 需要上锁
                        charger->server_send_status = MSG_STATE_NULL;
                        if (charger->present_status != CHARGER_STATE_RESTART)
                                charger->present_status = charger->present_mode;
                        if (charger->message) {
                             free(charger->message);
                             charger->message = NULL;
                        }
                        pthread_mutex_unlock(&new_task_mutex);
                break;
                
                case DASH_HB_INFO:
                        // 进度发送, 用errcode表示, 用于软件更新与抄表
                        printf("当前进度:%d%%\n", dash_info->errcode);
                break;
       
                case  DASH_ERRNO_INFO:
                         // 电桩状态
                         show_info(dash_info->errcode); 
                         if ( dash_info->errcode <= CHARGER_STATE_OFF_NET )
                         {
                        
                         } else if (dash_info->errcode >= MSG_STATE_NULL && dash_info->errcode <= MSG_STATE_CONTROL )
                         {
                                // 还处于发送命令状态
                         } else if (dash_info->errcode == FILE_NO_EXIST)
                         {
                                // 文件不存在
                         } else if (dash_info->errcode == FILE_SIZE_ZERO)
                         {
                                // 0文件
                         } else if (dash_info->errcode == CHARGER_NO_EXIST)
                         {
                                // 电桩不存在
                         } else if (dash_info->errcode == SERVER_DEAL_ERROR)
                         {
                                // 路由处理失败
                         } else 
                         {
                                // 未知错误
                         }
                         pthread_mutex_lock(&new_task_mutex);
                         if (charger && charger->message) {
                                free(charger->message);
                                charger->message = NULL;
                         }
                         pthread_mutex_unlock(&new_task_mutex);
                break;
       }
       free(dash_info); 


#if 0
        if (task->way == WEB_WAY)
        {
	             ev_uci_save_action(UCI_SAVE_OPT, true, "1", "chargerinfo.%s.STATUS", charger->tab_name); 
                    // 来自页面的操作
                     if( task->cmd ==  MSG_CHAOBIAO_FINISH)
                     {
                             sprintf(val_buff, "%ld", time(0));
	                     ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CB_END_TIME", charger->tab_name); 
	                     sprintf(val_buff, "%d", task->u.chaobiao.chargercode);
                             ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CB_NUM", charger->tab_name);
                     } else if (task->errcode == MSG_CONFIG_FINISH)
                     {
                            sprintf(val_buff, "%ld", time(0));
	                    ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CONF_END_TIME", charger->tab_name); 
                            close(charger->file_fd);
                            charger->present_status = MSG_STATE_NULL; 
                            free(task);
                            continue;
                     } else if (task->cmd == MSG_UPDATE_FINISH)
                     {
                           sprintf(val_buff, "%ld", time(0));
	                   ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.FW_END_TIME", charger->tab_name); 
	                   ev_uci_save_action(UCI_SAVE_OPT, true, "1", "chargerinfo.%s.STATUS", charger->tab_name); 
                           close(charger->file_fd);
                           charger->present_status = MSG_STATE_NULL;
                           free(task);
                           continue; 
                     }
        }
        // 不发送到后台的信息，自己处理。
        if (task->cmd == MSG_CMD_FROM_CHARGER)
        {
                if (task->errcode == EMSG_START_CHARGE_CLEAN)
                {
                         charger->server_send_status = MSG_STATE_NULL;
                         free(charger->start_charge_order);
                         free(charger->start_charge_package);
                         free(charger->uid);
                         free(task);
                         continue;
                } else if (task->errcode == EMSG_STOP_CHARGE_CLEAN)
                {
                         charger->server_send_status = MSG_STATE_NULL;
                         free(charger->uid);
                         task->cmd = WAIT_CMD_PUSH_MESSAGE_API;
                } else if (task->errcode == MSG_YUYUE_FINISH)
                {
                         charger->server_send_status = MSG_STATE_NULL;
                         free(charger->uid);
                         task->cmd = WAIT_CMD_RESERVED_API;
                } else if (task->errcode == MSG_CHAOBIAO_FINISH)
                {
                        charger->present_status = MSG_STATE_NULL; 
                        close(charger->file_fd);
                        task->cmd = WAIT_CMD_CHAOBIAO_API;
                } else if (task->errcode == MSG_START_CHARGE_FINISH)
                {
                        charger->server_send_status = MSG_STATE_NULL;
                        free(charger->uid);
                        
                } else if (task->errcode == EMSG_STOP_CHARGE_ERROR)
                {
                        charger->server_send_status = MSG_STATE_NULL;
                        free(charger->uid);
                        free(task);
                        continue;
                } else if (task->errcode == EMSG_START_CHARGE_ERROR )
                {
                       if (charger->server_send_status != MSG_STATE_NULL)
                       {
                             charger->server_send_status = MSG_STATE_NULL;
                             free(charger->start_charge_order);
                             free(charger->start_charge_package);
                             free(charger->uid);
                       }
                       task->cmd = WAIT_CMD_PUSH_MESSAGE_API;
                } else if (task->errcode == EMSG_YUYUE_ERROR)
                {
                       charger->server_send_status = MSG_STATE_NULL;
                       free(charger->uid);
                } 
        }
        if ( pthread_create(&tid, NULL, pthread_send_to_backstage, (void *)task) < 0)
               exit(1);
#endif
    }
}

// 读取文件版本号
struct upt *read_update_file_version(char *name)
{
    static  struct upt  update;
    char    main_v[5] = {0}, second_v[5] = {0},  *v1,  *v2,  *v3;

    if (( v1 = strchr(name, '_')) != NULL)
    {
        v2 = strchr(name, '.');
//    }else if ( ( v1 = strchr(name, 'v')) != NULL)
//    {
//        v2 = strchr(name, '.');
    } else
    {
        printf("不能识别的文件名,%s ...", name);
        return NULL;
    }
    v3 = strchr(v2 + 1, '.');
    if (v3 == NULL || v2 == NULL) {
        printf("文件解析为空。。。\n");
        return NULL;
    }
    strncpy(main_v, v1+2, v2 - v1 - 1);
    strncpy(second_v, v2+1, v3 - v2 - 1);
    debug_msg("main_v:%s second_v:%s", main_v, second_v);
    update.version[0] = atoi(main_v);
    update.version[1] = atoi(second_v);
    strcpy(update.name, name);
   
    return &update;
}

// 协议解析函数
// A(1Byte) + B(2Byte) + C(2Byte) + D(nbyte)
struct NewTask *parse_server_command(const char *command)
{
    static  struct NewTask  new_task;
    char    val[40] = {0}, A, B, C, offset; 
    time_t  init_time = 1262275200; // 2010-01-01

    A = *command - '0';
    B = atoi(strncpy(val, command + 1, 2));
    C = atoi(strncpy(val, command + 3, 2));

    printf("A:%d, B:%d, C:%d ", A, B, C);
    switch ( command[0] )
    {
        case    '1':    // update
                        if (2 == B) // 电桩更新
                        {
                                char    *fpos;
                                
                                memset(val, 0, strlen(val));
                                if (1 == C) // 部分更新
                                {
                                        new_task.cmd = WAIT_CMD_ONE_UPDATE;
                                        new_task.cid = atoi(strncpy(val, command +5, 8));   //cid
                                        offset = 13;

                                } else if (2 == C) // 全更新
                                {
                                        new_task.cmd = WAIT_CMD_ALL_UPDATE;
                                        offset = 5;
                                }
                                // 解析版本号，和文件名字
                                fpos = strchr(command + offset, ' ');
                                strncpy(val, command + offset, fpos - (command + offset));
                                debug_msg("后台发送的文件名:%s\n", val);
                                struct upt *update = read_update_file_version(val);
                                if (update == NULL)
                                 {
                                       return NULL;
                                 }
                                 memcpy(&new_task.u.update, update, sizeof(struct upt));

                        } else if (4 == B) // 配置更新
                        {
                                // 没有C
                                new_task.cmd = WAIT_CMD_CONFIG;
                                new_task.cid = atoi(strncpy(val, command +5, 8));    //cid
                                sprintf(new_task.u.config.name, "%08d", new_task.cid);
                        }
        break;
        case    '2':    // chaobiao
                        if ( 1 == B) // one chaobiao
                        {
                                new_task.cmd = WAIT_CMD_CHAOBIAO;
                                new_task.cid = atoi(strncpy(val, command + 5, 8));
                                offset = 13;
                        }
                         else if ( 2 == B)
                         {
                                new_task.cmd = WAIT_CMD_ALL_CHAOBIAO;
                                offset = 5;
                         }
                        if ( 1== C) // chaobiao none time
                        {
                         //       new_task.u.chaobiao.start_time = atoi(strncpy(val, command + 13, 10));
                         //       new_task.u.chaobiao.end_time = atoi(strncpy(val, command + 23, 10));
                        } else if ( 2 == C)
                        {
                                new_task.u.chaobiao.start_time = atoi(strncpy(val, command + offset, 10));
                                new_task.u.chaobiao.end_time = atoi(strncpy(val, command + offset +10, 10));
                        } 

        break;

        case    '3':    // charge
                 if (1 == B)  // start charge
                 {
                         new_task.cmd = WAIT_CMD_START_CHARGE;
                         new_task.cid = atoi(strncpy(val, command +5, 8));   //cid
                         memcpy(new_task.u.start_charge.uid, command + 13, 32); // uid
                         memset(val, 0, strlen(val)); //energy
                         new_task.u.start_charge.energy = atoi(strncpy(val, command + 45, 4));
                         memset(new_task.u.start_charge.order_num, ' ', sizeof(new_task.u.start_charge.order_num)); //order
                         memcpy(new_task.u.start_charge.order_num, command + 49, 21);
                         memcpy(new_task.u.start_charge.package, command + 70, 15); //package
                 
                 } else if (2 == B) // stop charge
                 {
                        new_task.cmd = WAIT_CMD_STOP_CHARGE;
                        new_task.cid = atoi(strncpy(val, command +5, 8));   //cid
                        new_task.u.stop_charge.username = C; // 1--->admin; 2--->gernal users
                        if (1 == C) //  admin
                        {
                        }
                        else if (2 == C)
                        {
                                memcpy(new_task.u.stop_charge.uid, command +13, 32);    // uid
                                debug_msg("uid:%s\n", new_task.u.stop_charge.uid);
                        }

                 } else if (3 == B) // reboot charger
                 {
                        new_task.cmd = WAIT_CMD_CTRL;
                        new_task.cid = atoi(strncpy(val, command +5, 8));   //cid
                        new_task.u.control.value = 1; // 重启value
                 } else if (4 == B) // floor lock
                 {
                        new_task.cmd = WAIT_CMD_CTRL;
                        new_task.cid = atoi(strncpy(val, command + 5, 8)); // cid
                        new_task.u.control.value = 2 + C;                  // up or down
                 }

        break;
        case    '4':    // yuyue
                 new_task.cmd = WAIT_CMD_YUYUE;
                 new_task.cid = atoi(strncpy(val, command + 5, 8));
                 if (B == 1)
                 {
                    new_task.u.yuyue.time = 30;
                 } else if (B == 2)
                 {
                    new_task.u.yuyue.time = 0;
                 }
                 memcpy(new_task.u.yuyue.uid, command + 13, 32);

        break;
        default:
                return NULL;
    }
    printf("udp服务器发送过来的命令:cid:%d, cmd%d", new_task.cid, new_task.cmd);
    return (&new_task);
}


void check_all_callback(void *value, void *arg)
{
        struct PipeTask   pipe_task;
        struct NewTask    *new_task = (struct NewTask *)arg;
        CHARGER_INFO_TABLE *charger = (CHARGER_INFO_TABLE *)value;
       
        if ( new_task->cmd == WAIT_CMD_ALL_UPDATE)
            new_task->cmd = WAIT_CMD_ONE_UPDATE;
       else if (new_task->cmd == WAIT_CMD_ALL_CHAOBIAO)
           new_task->cmd = WAIT_CMD_CHAOBIAO;
       new_task->cid = charger->CID;
       if ( check_have_one_handle(charger, new_task, finish_task_add) < 0)
            return;
       pipe_task.cmd = new_task->cmd;
       pipe_task.cid = charger->CID;
       pipe_task.way =  new_task->way;
       writen(new_task->pipefd, &pipe_task, sizeof(struct PipeTask));

       return;
}
int    check_have_all_handle(int pipefd, struct NewTask *new_task)
{
        int i;
        
        if (new_task->cmd != WAIT_CMD_ALL_UPDATE && new_task->cmd != WAIT_CMD_ALL_CHAOBIAO)
                return 0;
        
        // 遍历所有电桩
        new_task->pipefd = pipefd;
        hash_foreach_entry(global_cid_hash, check_all_callback, new_task);
        
        return 1;
}


int    check_have_one_handle(CHARGER_INFO_TABLE *charger, struct NewTask *task, 
       void (*commit_wrong)(int, const CHARGER_INFO_TABLE *, const char *, int))
{
    char  str_buff[1024];
    struct stat *st;
    int err_code;        
//    CHARGER_INFO_TABLE *charger, tmp_charger;
    
    charger->message = NULL; 
    if ( charger == NULL)
    {
        //操作没有该电桩
        err_code = CHARGER_NO_EXIST;
        goto  commit;
    }
    if (charger->present_status == CHARGER_STATE_OFF_NET)
    {
        err_code = CHARGER_STATE_OFF_NET;
        goto commit;
    }
    printf("charger->server_send_status:%d\n", charger->server_send_status);
#if 1
    // 检查上一次的命令的处理
    pthread_mutex_lock(&new_task_mutex);
    if (   charger->server_send_status == MSG_STATE_CONFIG  || 
           charger->server_send_status == MSG_STATE_UPDATE  || 
           charger->server_send_status == MSG_STATE_CHAOBIAO )
     {
        if ( (time(0) - charger->last_update_time) < 5 *60) // 5min
        {
            err_code = charger->server_send_status;
            pthread_mutex_unlock(&new_task_mutex);
            goto commit;
        }
        // 线程同步
        if (charger->message) 
                close(charger->message->file_fd);
    } else if  (charger->server_send_status == MSG_STATE_YUYUE ||
               charger->server_send_status == MSG_STATE_START_CHARGE || 
               charger->server_send_status == MSG_STATE_STOP_CHARGE
              )
    {
        if ( (time(0) - charger->last_update_time) < 10) // 10sec
        {
            err_code = charger->server_send_status;
            pthread_mutex_unlock(&new_task_mutex);
            goto commit;
        } 
        charger->server_send_status = MSG_STATE_NULL;
    } 
    // 注意上锁,目前没有上锁
    if (charger->message)
    {
        free(charger->message);
        charger->message = NULL;
    }
    pthread_mutex_unlock(&new_task_mutex);
    if ( !(charger->message = (struct Message *)malloc(sizeof(struct Message))) ) {
        err_code = SERVER_DEAL_ERROR;
        goto commit;
    }
    charger->message->new_task = *task;  
#endif
    // 此时的命令
    
    printf("charger->present_status:%d\n", charger->present_status);
    if (task->cmd == WAIT_CMD_CONFIG)
    {
        if (charger->present_status != CHARGER_READY)
        {
                // 电桩无法推送配置
                err_code = charger->present_status;
                goto commit;
        }
        sprintf(str_buff, "%s/%s/%s", WORK_DIR, CONFIG_DIR, task->u.config.name);
        printf("config:%s\n", str_buff);
        if (access(str_buff, F_OK) != 0)
        {
                err_code = FILE_NO_EXIST;
                goto  commit;
        }
        if ( (st = (struct stat *)malloc(sizeof(struct stat))) == NULL)
         {
                err_code = SERVER_DEAL_ERROR;
                goto  commit;
         }
         if (stat(str_buff, st) < 0)
         {
                err_code = SERVER_DEAL_ERROR;
                free(st);
                goto  commit;
         }
         if (st->st_size == 0) {
                free(st);
                err_code = FILE_SIZE_ZERO;
                goto commit;
         }
         charger->message->file_length = st->st_size;
         if (charger->message->file_length & 1023 == 0)
                charger->message->file_package = charger->message->file_length / 1024;
          else
                charger->message->file_package = charger->message->file_length / 1024 + 1;
          if ( (charger->message->file_fd = open(str_buff, O_RDONLY, 0400)) < 0)
         {
                err_code = SERVER_DEAL_ERROR;
                free(st);
                goto  commit;
         }
        charger->message->file_length = st->st_size;
        charger->server_send_status = MSG_STATE_CONFIG;
        free(st);

    } else if (task->cmd == WAIT_CMD_CHAOBIAO)
    {
        if (charger->present_status != CHARGER_READY && charger->present_status != CHARGER_CHARGING)
        {
                err_code = charger->present_status;
                goto  commit;
        }
        if (task->way == SERVER_WAY)
            sprintf(str_buff, "%s/%s/%08d_server", WORK_DIR, CHAOBIAO_DIR, task->cid);
        else
            sprintf(str_buff, "%s/%s/%08d_web", WORK_DIR, CHAOBIAO_DIR, task->cid);
        
        unlink(str_buff);
        if ( (charger->message->file_fd = creat(str_buff, FILEPERM)) < 0)
        {
               err_code = SERVER_DEAL_ERROR;
                goto  commit;
        }
        charger->server_send_status = MSG_STATE_CHAOBIAO;

    } else if (task->cmd == WAIT_CMD_ONE_UPDATE)
    {
        if (charger->present_status != CHARGER_READY)
        {
                err_code = charger->present_status;
                goto  commit;
        }
        sprintf(str_buff, "%s/%s/%s", WORK_DIR, UPDATE_DIR, task->u.update.name);
        printf("file:%s\n", str_buff);
        if ( access(str_buff, F_OK) != 0)
        {
               err_code = SERVER_DEAL_ERROR;
                goto  commit;
        }
        if ( (st = (struct stat *)malloc(sizeof(struct stat))) == NULL)
         {
              err_code = SERVER_DEAL_ERROR;
                goto  commit;
         }
         if (stat(str_buff, st) < 0)
         {
              err_code = SERVER_DEAL_ERROR;
                free(st);
                goto  commit;
         }
             if ( (charger->message->file_fd = open(str_buff, O_RDONLY, 0400)) < 0)
         {
              err_code = SERVER_DEAL_ERROR;
                free(st);
                goto  commit;
         }
          if (st->st_size == 0) {
                err_code = FILE_SIZE_ZERO;
                free(st);
                goto commit;
          }
         charger->message->file_length = st->st_size;
         if (charger->message->file_length & 1023 == 0)
                charger->message->file_package = charger->message->file_length / 1024;
          else
                charger->message->file_package = charger->message->file_length / 1024 + 1;
         charger->server_send_status = MSG_STATE_UPDATE;
         free(st);
        printf("update_file:%s\n", str_buff);
        printf("file_length:%d\n", charger->message->file_length);

    } else if (task->cmd == WAIT_CMD_YUYUE)
    {
        if (charger->present_status != CHARGER_READY)
        {
               err_code = charger->present_status;
                goto  commit;
        }
        charger->server_send_status = MSG_STATE_YUYUE;

    } else if (task->cmd == WAIT_CMD_START_CHARGE)
    {
          if (charger->present_status == CHARGER_CHARGING)
          {
               err_code = charger->present_status;
                goto  commit;
          }
          charger->server_send_status = MSG_STATE_START_CHARGE;

    } else if (task->cmd == WAIT_CMD_STOP_CHARGE)
    {
          if (charger->present_status == CHARGER_READY)
          {
                err_code = charger->present_status;
                goto  commit;
          }
          charger->server_send_status = MSG_STATE_STOP_CHARGE;
          printf("stop  cmd ...\n");
    } else if (task->cmd == WAIT_CMD_CTRL)
    {
          charger->server_send_status = MSG_STATE_CONTROL;
          printf("control  cmd...\n");
    }
    charger->last_update_time = time(0);
    return 0;

commit:
      printf("commit error...\n");
      if (commit_wrong)
                (*commit_wrong)(DASH_ERRNO_INFO, charger, NULL, err_code);
    return -1;
}

void exec_network(void *value, void *arg)
{
        CHARGER_INFO_TABLE *charger = (CHARGER_INFO_TABLE *)value;

        if (time(0) - charger->connect_time > 600) // 10 mins off network
        {
            ev_uci_save_action(UCI_SAVE_OPT, true, "46", "chargerinfo.%08d.PresentMode", charger->CID);
            charger->present_status = CHARGER_STATE_OFF_NET;
        }
}
void   check_charger_network(void)
{
        hash_foreach_entry(global_cid_hash, exec_network, NULL);
}

void * pthread_service_receive(void *arg)
{
    DIR *dir;
    struct  dirent  *ent = NULL;
    char    *recv_str;
    int CMD = 0, CID = 0, CID_TMP, pipefd;
    char    i;
    char    have_handle_flag, val_buff[250] = {0};
    time_t   init_time = 1262275200;
    char    CMD_TMP;
    struct  NewTask   new_task, *p_new_task;
    struct  PipeTask  pipe_task;
    struct  upt     *update;
    CHARGER_INFO_TABLE  *charger;
    // 初始化chargerinfo命令
    if ( (pipefd = open(PIPEFILE, O_WRONLY, 0200)) < 0){
            exit(EXIT_FAILURE);
     }
   for ( ; ; )
   {
            // 阻塞住，每5秒超时
       recv_str = mqreceive_timed("/server.cmd", 100, 30);
       if (recv_str)
       {
                    debug_msg("队列有数据, 正在操作，str =%s...", recv_str);
#if 1
                    if ( (p_new_task = parse_server_command(recv_str)) == NULL)
                    {
                            free(recv_str);
                            continue;
                    }
                    p_new_task->way = SERVER_WAY;
                    if ( check_have_all_handle(pipefd, p_new_task))
                    {
                            free(recv_str);
                            continue;
                    }
                    charger = look_for_charger_from_array(p_new_task->cid);
                    if ( check_have_one_handle(charger, p_new_task, finish_task_add) < 0)
                    {
                         free(recv_str);
                         continue;
                    }
                    pipe_task.cid = p_new_task->cid;
                    pipe_task.cmd = p_new_task->cmd;
                    writen(pipefd, &pipe_task, sizeof(struct PipeTask));
                    free(recv_str);
                    continue;
#endif
          }

         check_charger_network();
         if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.CID") < 0) //CMD 
            goto set_zero;
         CID = atoi(val_buff);
         if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.CMD") < 0) //CMD 
            goto set_zero;
         CMD = atoi(val_buff);

      printf(" dummery CID = %d, CMD= %d\n", CID, CMD); 
      new_task.cid = CID;
      new_task.cmd = CMD;
        if (CMD != WAIT_CMD_ALL_CHAOBIAO && CMD != WAIT_CMD_ALL_UPDATE)
        {
                if (CMD == 0 || CID == 0)
                        continue;
                if ( !(charger = look_for_charger_from_array(new_task.cid)) )
                        goto set_zero;
        }
        new_task.way = WEB_WAY;
        new_task.pipefd = pipefd;
        p_new_task = &new_task;
        printf("CID = %d, CMD= %d\n", CID, CMD); 
        
      // 封装命令
      switch (CMD)
      {
        case    WAIT_CMD_YUYUE:
                ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.uid");
                strncpy(p_new_task->u.yuyue.uid, val_buff, 32);
                p_new_task->u.yuyue.time = 30;
        break;

        case    WAIT_CMD_CHAOBIAO:
                    new_task.u.chaobiao.start_time = init_time;
                    new_task.u.chaobiao.end_time = time(0);//end_time;
        break;
        case    WAIT_CMD_CONFIG:
                    sprintf(val_buff, "%08d.config", CID);
                    strcpy(new_task.u.config.name, val_buff);
        break;
        case    WAIT_CMD_ONE_UPDATE:
        case    WAIT_CMD_ALL_UPDATE:
                    // 搜索文件
                    sprintf(val_buff, "%s/%s", WORK_DIR, UPDATE_DIR);
                    if ( (dir = opendir(val_buff)) == NULL)
                            goto set_zero;
                    while ( (ent = readdir(dir)) )
                    {
                           if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                                    continue;
                           strcpy(val_buff, ent->d_name);
                                    break;
                    }
                    closedir(dir);
                    printf("read file = %s\n", val_buff);
                    update = read_update_file_version(val_buff);
                    if (update == NULL)
                            goto set_zero;
                    memcpy(&new_task.u.update, update, sizeof(struct upt));
                    if ( check_have_all_handle(pipefd, p_new_task))
                        goto set_zero;
//                    charger = look_for_charger_from_array(p_new_task->cid);
//                    if ( check_have_one_handle( charger, p_new_task, NULL) < 0)
//                           goto set_zero;
//                    pipe_task.cid = p_new_task->cid;
//                    pipe_task.cmd = p_new_task->cmd;
//                    writen(pipefd, &pipe_task, sizeof(struct PipeTask));

                    printf("pthread of service read update file = %s\n", val_buff);
        break;
        case    WAIT_CMD_ALL_CHAOBIAO:
                new_task.u.chaobiao.start_time = time(0) - 7 * 60 *60 *24;
                new_task.u.chaobiao.end_time = time(0);//end_time;
                if ( check_have_all_handle(pipefd, p_new_task) )
                    goto set_zero;
        break;
        case    WAIT_CMD_START_CHARGE:
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.uid") < 0) //CMD 
                {  
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.uid");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.package");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.order");
                    goto set_zero;
                }
                if (strlen(val_buff) != 32)
                {
                    debug_msg("uid 长度不够 ...");
                    goto set_zero;
                }
                strncpy(new_task.u.start_charge.uid, val_buff, 32);
                printf("uid = %s\n", val_buff);
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.order") < 0) //CMD 
                {
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.uid");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.package");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.order");
                    goto set_zero;
                }
                memcpy(val_buff + strlen(val_buff), "                         ", 
                        sizeof(new_task.u.start_charge.order_num) - strlen(val_buff));
                strncpy(new_task.u.start_charge.order_num, val_buff, sizeof(new_task.u.start_charge.order_num));
                printf("order = %s\n", new_task.u.start_charge.order_num);
                 
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.package") < 0) //CMD 
                {
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.uid");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.package");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.order");
                    goto set_zero;
                }
                if (strlen(val_buff) > 15)
                    goto set_zero;
                // 追加空格
                memcpy(val_buff + strlen(val_buff), "               ", sizeof(new_task.u.start_charge.package) - strlen(val_buff));
                strncpy(new_task.u.start_charge.package, val_buff, sizeof(new_task.u.start_charge.package));
                printf("package = %s\n", new_task.u.start_charge.package);
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.energy") < 0) //CMD
                { 
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.uid");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.package");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.order");
                    goto set_zero;
                }
                new_task.u.start_charge.energy = atoi(val_buff);
                printf("energy = %s\n", val_buff);

        break;
        case    WAIT_CMD_STOP_CHARGE:
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.uid") < 0) //CMD 
                {   
                      ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.uid");
                      goto set_zero;
                }
                if(strlen(val_buff) != 32)
                {
                    debug_msg("uid 长度不够 ...");
                    goto set_zero;
                }
                strncpy(new_task.u.stop_charge.uid, val_buff, 32);
        break;
        default:
            goto set_zero;
        break;
      }
      // 加入等待队列
       if ( check_have_one_handle( charger, p_new_task, NULL) < 0)
              goto set_zero;
       pipe_task.cid = p_new_task->cid;
       pipe_task.cmd = p_new_task->cmd;
       writen(pipefd, &pipe_task, sizeof(struct PipeTask));
        printf("pthread of service add task queue success");
set_zero:
        ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.CLIENT.STATUS"); //
        sprintf(val_buff, "%d", 0);
	ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CMD");
        sprintf(val_buff, "%08d", CID);
	ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.CLIENT.CID");
        ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.CMD");
        CMD = CID = 0; 
   } //end for
    return NULL;
}

void *load_balance_pthread(void *arg)
{

}

static int uci_query(const char *uci_value, void *arg UNUSED)
{
        char *su, *mac;
        char cid[50], buff[50];
        int id, present_mode;
        CHARGER_INFO_TABLE charger;

        su = strchr(uci_value, '@');
        memcpy(cid, uci_value, su - uci_value);
        cid[su - uci_value] = 0;
        printf("cid:%s\n", cid);

        if ( ev_uci_data_get_val(buff, sizeof(buff), "chargerinfo.%s.CID", cid) < 0)
                exit(1);
        id = atoi(buff);
        printf("id=%d\n", id);
        if ( ev_uci_data_get_val(buff, sizeof(buff), "chargerinfo.%s.MAC", cid) < 0)
                exit(1);
         strcpy(charger.MAC, buff);
//        if ( ev_uci_data_get_val(buff, sizeof(buff), "chargerinfo.%s.PresentMode", cid) < 0)
//                exit(1);
//        present_mode = atoi(buff);
       charger.CID = id; 
        hash_add_entry(global_cid_hash, &id, sizeof(id), &charger, sizeof(charger));
        
        return 0;
}

//取出数据库配置信息，然后存入内存中
void  charger_info_init(int select)
{

        if (select)
        {
                ev_uci_save_val_string("0", "chargerinfo.SERVER.CID");
                ev_uci_save_val_string("0", "chargerinfo.SERVER.CMD");
                ev_uci_save_val_string("0", "chargerinfo.SERVER.order");
                ev_uci_save_val_string("0", "chargerinfo.SERVER.package");
                ev_uci_save_val_string("0", "chargerinfo.SERVER.energy");
                ev_uci_save_val_string("0", "chargerinfo.SERVER.uid");
                ev_uci_save_val_string("0", "chargerinfo.SERVER.START_TIME");
                ev_uci_save_val_string("0", "chargerinfo.SERVER.END_TIME");
                return;
        }
        
        ev_uci_list_foreach("chargerinfo.chargers.cids", uci_query, NULL);
        return ;
}

