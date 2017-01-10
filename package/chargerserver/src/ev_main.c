

//基于TCP的EV充电桩与EV_Router通信 服务器端
#include "include/err.h"
#include "include/CRC.h"
#include "include/AES.h"
#include "include/list.h"
#include "include/ev_lib.h"
#include <libev/msgqueue.h>
#include "include/serv_config.h"
#include "include/ev_command_file.h"

pthread_mutex_t		clifd_mutex = PTHREAD_MUTEX_INITIALIZER;	// 线程互斥锁初始化
//pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;  // 线程条件变量初始化
pthread_mutex_t		serv_mutex = PTHREAD_MUTEX_INITIALIZER;	        // 线程互斥锁初始化
pthread_cond_t		serv_cond = PTHREAD_COND_INITIALIZER;		// 线程条件变量初始化
pthread_rwlock_t	charger_rwlock;

// 充电桩信息，数组
CHARGER_INFO_TABLE	ChargerInfo[15];	                        // 用于存入每台电桩信息	
CHARGER_MANAGER		charger_manager = {
	.total_num = 8,
	.limit_max_current = 56,
	.present_off_net_cnt = 8,
};
sigset_t        mask;

// 函数声明
void thread_make(ev_int i);
ev_int doit_charger_serv(const ev_int fd, const ev_int cmd, CHARGER_INFO_TABLE *charger,  BUFF *bf);
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
struct wait_task *parse_server_command(const char *command);
int    check_have_all_handle(int pipefd, char way, struct wait_task *task);
int    check_have_one_handle(char, CHARGER_INFO_TABLE *, struct wait_task *, void (*commit_wrong)(int, struct finish_task *));


void  dir_init(void)
{
    char    name[100];
    mkdir(WORK_DIR, 0711);
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
                ev_uci_add_named_sec("chargerinfo.%s=tab", "TABS");
//		ev_uci_add_named_sec("chargerinfo.%s=rod", "Record");
                ev_uci_add_named_sec("chargerinfo.%s=issued", "SERVER");
                ev_uci_add_named_sec("chargerinfo.%s=respond", "CLIENT");
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
        
        printf("%s start run ...\n", argv[0]);
        opt_type = load_command_line(argc, argv);
        printf("opt_type:%#x\n", opt_type); 
        command_init_for_run(opt_type, 
                                       OPTION_HELP | OPTION_VERSION | 
                                       OPTION_DAEMON | OPTION_START | 
                                       OPTION_STOP | OPTION_RESTART | 
                                       OPTION_DEBUG | OPTION_PROCESSLOCK);
        // controller初始化
        protocal_controller_init();
       
        // 服务器地址初始化
        listenfd = sock_serv_init();
        // 目录初始化
        dir_init(); 
        // 信号初始化
        signal_init();
        // 数据库初始化
        uci_database_init();
//      	command_init_for_run(opt_type, OPTION_LOADBALANCE);
        // 长连接下管道初始化
        unlink(PIPEFILE);
        if (mkfifo(PIPEFILE, 0777) < 0)
              err_sys("mkfifo %s failed", PIPEFILE);
        printf("pipefd = %d\n", pipefd);
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
#define MAX_LEN     1500

void send_cmd_to_chargers(struct join_wait_task *task)
{
        int cmd;
        CHARGER_INFO_TABLE      *charger;
        
        charger = look_for_charger_from_array(task->cid);
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

             cmd = CHARGER_CMD_AUTH_R;
        }
        else if (task->cmd == WAIT_CMD_CONFIG)
            cmd = CHARGER_CMD_CONFIG_R;
        else if (task->cmd == WAIT_CMD_YUYUE)
            cmd = CHARGER_CMD_YUYUE_R;
        else if (task->cmd == WAIT_CMD_START_CHARGE)
            cmd = CHARGER_CMD_START_CHARGE_R;
        else if (task->cmd == WAIT_CMD_STOP_CHARGE)
            cmd = CHARGER_CMD_STOP_CHARGE_R;
        else if (task->cmd == WAIT_CMD_ONE_UPDATE)
            cmd = CHARGER_CMD_START_UPDATE_R;
        else if (task->cmd == WAIT_CMD_CTRL)
            cmd = CHARGER_CMD_CTRL_R;
        else if (task->cmd == WAIT_CMD_CHAOBIAO)
            cmd = CHARGER_CMD_CHAOBIAO_R;
        start_service(charger->sockfd, NULL, cmd, charger->CID);
//        do_serv_run(charger->sockfd, cmd, bf, charger, error_handler);
}
typedef struct String {
        char *value;
        int  num;
}string;

void evdoit_l_serv(void)
{
        string          buff = {0};
        int             maxfd, i, connfd, sockfd, n, pipefd;
        int             nready, client[FD_SETSIZE];
        fd_set          rset, allset;
        struct join_wait_task   join_task;
        
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
                        if ( (n = read(pipefd, &join_task, sizeof(struct join_wait_task))) <= 0){
                             exit(EXIT_FAILURE); 
                        }
                        if (n != sizeof(struct join_wait_task))
                            continue;
                        // 发送命令
                         send_cmd_to_chargers(&join_task);
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
                                if ( (n = read(sockfd, buff.value, MAX_LEN)) <= 0)
                                {
                                        close(sockfd);
                                        FD_CLR(sockfd, &allset);
                                        client[i] = -1;
                                } else
                                {
                                        buff.num = n;
                                        buff.value[n] = '\0';
                                        start_service(sockfd, buff.value, 0, 0);
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
        int i; 
        int charger_cnt = charger_manager.present_charger_cnt;

        for (i = 0; i < charger_cnt; i++){
              if (ChargerInfo[i].CID == cid)
                     break;
        }
        if (i >= charger_cnt)
            return NULL;

        return ( &ChargerInfo[i] );  
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
//        int keepinterval = 5; //探测时发包的时间间隔
//        int keepcount = 3;    //探测时尝试的次数，成功后就不再发送
        Setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&keepalive, sizeof(keepalive));  // keepalive
        Setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (const void *)&keepidle, sizeof(keepidle));
//        Setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (const void *)&keepinterval, sizeof(keepinterval));
//        Setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT,  (const void *)&keepcount, sizeof(keepcount));
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
        struct finish_task *task;
        struct join_wait_task  wait_task;
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
                     wait_task.value = SYM_OFF_NET;
              else
                      wait_task.value = sptr[8] - 48;
                
              wait_task.cid = task->cid;
//              wait_task.cmd = WAIT_CMD_CHARGE_REQ;
              write(pipefd, &wait_task, sizeof(struct join_wait_task));
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
       return NULL; 
} //end pthread_send_to_backstage

void *pthread_service_send(void *arg)
{
    struct finish_task   *task;
    struct join_wait_task wait_task;
    int i;
    char   val_buff[200];
    pthread_t  tid;
    CHARGER_INFO_TABLE *charger;

    debug_msg("pthread of send message is running");
    for ( ; ;)
    {
        pthread_mutex_lock(&finish_task_lock);
        while (finish_head == NULL)
            pthread_cond_wait(&finish_task_cond, &finish_task_lock);
        // 移出节点
        finish_task_remove_head(task = finish_head);
        pthread_mutex_unlock(&finish_task_lock);

        // 格式化数据，发送给服务器
        // 写入luci数据库
        debug_msg("pthread of sending finish deal commands finish");
        printf("task->cmd = %d, cid = %d, way = %d, errcode = %d \n", 
                task->cmd, task->cid, task->way, task->errcode);

        charger = look_for_charger_from_array(task->cid);
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
        debug_msg("不能识别的文件名,%s ...", name);
        return NULL;
    }
    v3 = strchr(v2 + 1, '.');
    if (v3 == NULL || v2 == NULL)
        return NULL;
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
struct wait_task *parse_server_command(const char *command)
{
    static  struct wait_task   cmd;
    char     *val, A, B, C, offset; 
    time_t  init_time = 1262275200; // 2010-01-01

    if ( (val = (char *)calloc(40, sizeof(char))) ==NULL)
        return NULL;
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
                                        cmd.cmd = WAIT_CMD_ONE_UPDATE;
                                        cmd.cid = atoi(strncpy(val, command +5, 8));   //cid
                                        offset = 13;

                                } else if (2 == C) // 全更新
                                {
                                        cmd.cmd = WAIT_CMD_ALL_UPDATE;
                                        offset = 5;
                                }
                                // 解析版本号，和文件名字
                                fpos = strchr(command + offset, ' ');
                                strncpy(val, command + offset, fpos - (command + offset));
                                debug_msg("后台发送的文件名:%s\n", val);
                                struct upt *update = read_update_file_version(val);
                                if (update == NULL)
                                 {
                                       free(val);
                                       return NULL;
                                 }
                                 memcpy(&cmd.u.update, update, sizeof(struct upt));

                        } else if (4 == B) // 配置更新
                        {
                                // 没有C
                                cmd.cmd = WAIT_CMD_CONFIG;
                                cmd.cid = atoi(strncpy(val, command +5, 8));    //cid
                                sprintf(cmd.u.config.name, "%08d", cmd.cid);
                        }
        break;
        case    '2':    // chaobiao
                        if ( 1 == B) // one chaobiao
                        {
                                cmd.cmd = WAIT_CMD_CHAOBIAO;
                                cmd.cid = atoi(strncpy(val, command + 5, 8));
                                offset = 13;
                        }
                         else if ( 2 == B)
                         {
                                cmd.cmd = WAIT_CMD_ALL_CHAOBIAO;
                                offset = 5;
                         }
                        if ( 1== C) // chaobiao none time
                        {
                         //       cmd.u.chaobiao.start_time = atoi(strncpy(val, command + 13, 10));
                         //       cmd.u.chaobiao.end_time = atoi(strncpy(val, command + 23, 10));
                        } else if ( 2 == C)
                        {
                                cmd.u.chaobiao.start_time = atoi(strncpy(val, command + offset, 10));
                                cmd.u.chaobiao.end_time = atoi(strncpy(val, command + offset +10, 10));
                        } 

        break;

        case    '3':    // charge
                 if (1 == B)  // start charge
                 {
                         cmd.cmd = WAIT_CMD_START_CHARGE;
                         cmd.cid = atoi(strncpy(val, command +5, 8));   //cid
                         memcpy(cmd.u.start_charge.uid, command + 13, 32); // uid
                         memset(val, 0, strlen(val)); //energy
                         cmd.u.start_charge.energy = atoi(strncpy(val, command + 45, 4));
                         memset(cmd.u.start_charge.order_num, ' ', sizeof(cmd.u.start_charge.order_num)); //order
                         memcpy(cmd.u.start_charge.order_num, command + 49, 21);
                         memcpy(cmd.u.start_charge.package, command + 70, 15); //package
                 
                 } else if (2 == B) // stop charge
                 {
                        cmd.cmd = WAIT_CMD_STOP_CHARGE;
                        cmd.cid = atoi(strncpy(val, command +5, 8));   //cid
                        cmd.u.stop_charge.username = C; // 1--->admin; 2--->gernal users
                        if (1 == C) //  admin
                        {
                        }
                        else if (2 == C)
                        {
                                memcpy(cmd.u.stop_charge.uid, command +13, 32);    // uid
                                debug_msg("uid:%s\n", cmd.u.stop_charge.uid);
                        }

                 } else if (3 == B) // reboot charger
                 {
                        cmd.cmd = WAIT_CMD_CTRL;
                        cmd.cid = atoi(strncpy(val, command +5, 8));   //cid
                        cmd.u.control.value = 1; // 重启value
                 } else if (4 == B) // floor lock
                 {
                        cmd.cmd = WAIT_CMD_CTRL;
                        cmd.cid = atoi(strncpy(val, command + 5, 8)); // cid
                        cmd.u.control.value = 2 + C;                  // up or down
                 }

        break;
        case    '4':    // yuyue
                 cmd.cmd = WAIT_CMD_YUYUE;
                 cmd.cid = atoi(strncpy(val, command + 5, 8));
                 if (B == 1)
                 {
                    cmd.u.yuyue.time = 30;
                 } else if (B == 2)
                 {
                    cmd.u.yuyue.time = 0;
                 }
                 memcpy(cmd.u.yuyue.uid, command + 13, 32);

        break;
        default:
                free(val);
                return NULL;
    }
    printf("udp服务器发送过来的命令:cid:%d, cmd%d", cmd.cid, cmd.cmd);
    free(val);
    return (&cmd);
}

int    check_have_all_handle(int pipefd, char way, struct wait_task *task)
{
        int i;
        struct join_wait_task   join_task;
        
        if (task->cmd != WAIT_CMD_ALL_UPDATE && task->cmd != WAIT_CMD_ALL_CHAOBIAO)
        {
                return 0;
        }       
        for ( i = 0; i < charger_manager.present_charger_cnt; i++)
        {
                if (ChargerInfo[i].present_status != EMSG_OFF_NET)
                {
                        if (task->cmd == WAIT_CMD_ALL_UPDATE )
                        {
                              task->cmd = WAIT_CMD_ONE_UPDATE;
                              join_task.cmd = WAIT_CMD_ONE_UPDATE;
                              if (check_have_one_handle(way, ChargerInfo + i, task, finish_task_add) < 0)
                                   continue;
                        } else if (task->cmd == WAIT_CMD_ALL_CHAOBIAO )
                        {
                              task->cmd = WAIT_CMD_CHAOBIAO;
                              join_task.cmd = WAIT_CMD_CHAOBIAO;
                              if (check_have_one_handle(way, ChargerInfo + i, task, finish_task_add) < 0)
                                   continue;
//                              do_serv_run(ChargerInfo[i].sockfd, CHARGER_CMD_CHAOBIAO_R, bf,  ChargerInfo + i, error_handler);
                        }
                        join_task.cid = ChargerInfo[i].CID;
                        join_task.way = way;
                        write(pipefd, &join_task, sizeof(struct join_wait_task));
                }
                
        }
        return 1;
}


int    check_have_one_handle(char way, CHARGER_INFO_TABLE *charger, struct wait_task *task, 
       void (*commit_wrong)(int, struct finish_task *))
{
    char  str_buff[1024];
    struct stat *st;
    struct finish_task fsh_task;
    struct finish_task *task_err = &fsh_task;
//    CHARGER_INFO_TABLE *charger, tmp_charger;
    
  
    if ( charger == NULL)
    {
        //操作没有该电桩
        task_err->errcode = EMSG_HAVE_NO_CHARGER_ERROR;
        task_err->cmd = task->cmd;
        goto  commit;
    }
    if (charger->present_status == MSG_OFF_NET)
    {
        task_err->errcode = EMSG_OFF_NET;
        goto commit;
    }
    printf("charger->server_send_status:%d\n", charger->server_send_status);
#if 1
    // 检查上一次的命令的处理
    if (charger->server_send_status == MSG_STATE_CONFIG || 
        charger->server_send_status == MSG_STATE_UPDATE || 
        charger->server_send_status == MSG_STATE_CHAOBIAO )
    {
        if ( (time(0) - charger->last_update_time) < 5 *60) // 5min
        {
            task_err->errcode = charger->server_send_status;
            goto commit;
        } 
        close(charger->file_fd);
    } else if (charger->server_send_status == MSG_STATE_YUYUE)
    {
        if ( (time(0) - charger->last_update_time) < 10) // 10sec
        {
            task_err->errcode = charger->server_send_status;
            goto commit;
        } 
        free(charger->uid);
        charger->server_send_status = MSG_STATE_NULL;
    } else if (charger->server_send_status == MSG_STATE_START_CHARGE)
    {
        if ( (time(0) - charger->last_update_time) < 10) // 10sec
        {
            task_err->errcode = charger->server_send_status;
            goto commit;
        } 
        free(charger->start_charge_order);
        free(charger->start_charge_package);
        free(charger->uid);
    } else if (charger->server_send_status == MSG_STATE_STOP_CHARGE)
    {
        if ( (time(0) - charger->last_update_time) < 10) // 10sec
        {
            task_err->errcode = charger->server_send_status;
            goto commit;
        } 
       free(charger->uid);
    }
#endif
    // 此时的命令
    
    printf("charger->present_status:%d\n", charger->present_status);
    if (task->cmd == WAIT_CMD_CONFIG)
    {
        if (charger->present_status != MSG_CHARGER_READY)
        {
                // 电桩无法推送配置
                task_err->errcode = charger->present_status;
                goto commit;
        }
        sprintf(str_buff, "%s/%s/%s", WORK_DIR, CONFIG_DIR, task->u.config.name);
        if (access(str_buff, F_OK) != 0)
        {
                task_err->errcode = EMSG_CONFIG_ERROR;
                goto  commit;
        }
        charger->file_length = st->st_size;
        charger->server_send_status = MSG_STATE_CONFIG;

    } else if (task->cmd == WAIT_CMD_CHAOBIAO)
    {
        if (charger->present_status != MSG_CHARGER_READY && charger->present_status != MSG_CHARGER_CHARGING)
        {
                task_err->errcode = charger->present_status;
                goto  commit;
        }
        if (way == SERVER_WAY)
            sprintf(str_buff, "%s/%s/%08d_server", WORK_DIR, CHAOBIAO_DIR, task->cid);
        else
            sprintf(str_buff, "%s/%s/%08d_web", WORK_DIR, CHAOBIAO_DIR, task->cid);
        
        unlink(str_buff);
        if ( (charger->file_fd = creat(str_buff, FILEPERM)) < 0)
        {
              task_err->errcode = EMSG_CHAOBIAO_ERROR;
                goto  commit;
        }
        charger->cb_start_time = task->u.chaobiao.start_time;
        charger->cb_end_time   = task->u.chaobiao.end_time; 
        charger->server_send_status = MSG_STATE_CHAOBIAO;

    } else if (task->cmd == WAIT_CMD_ONE_UPDATE)
    {
        if (charger->present_status != MSG_CHARGER_READY)
        {
                task_err->errcode = charger->present_status;
                goto  commit;
        }
        sprintf(str_buff, "%s/%s/%s", WORK_DIR, UPDATE_DIR, task->u.update.name);
        if ( access(str_buff, F_OK) != 0)
        {
              task_err->errcode = EMSG_UPDATE_ERROR;
                goto  commit;
        }
        if ( (st = (struct stat *)malloc(sizeof(struct stat))) == NULL)
         {
             task_err->errcode = EMSG_UPDATE_ERROR;
                goto  commit;
         }
         if (stat(str_buff, st) < 0)
         {
                task_err->errcode = EMSG_UPDATE_ERROR;
                free(st);
                goto  commit;
         }
         if ( (charger->file_fd = open(str_buff, O_RDONLY, 0400)) < 0)
         {
             task_err->errcode = EMSG_UPDATE_ERROR;
                free(st);
                goto  commit;
         }
         charger->file_length = st->st_size;
         charger->version[0]  = task->u.update.version[0];
         charger->version[1]  = task->u.update.version[1];
         charger->server_send_status = MSG_STATE_UPDATE;
         free(st);
        printf("update_file:%s\n", str_buff);
        printf("file_length:%d\n", charger->file_length);

    } else if (task->cmd == WAIT_CMD_YUYUE)
    {
        if (charger->present_status != MSG_CHARGER_READY)
        {
               task_err->errcode = charger->present_status;
                goto  commit;
        }
        if ( (charger->uid = (char *)malloc(32)) == NULL)
        {
                task_err->errcode = EMSG_YUYUE_ERROR;
                goto  commit;
        }
        charger->yuyue_time = task->u.yuyue.time;
        memcpy(charger->uid, task->u.yuyue.uid, 32); 
        charger->server_send_status = MSG_STATE_YUYUE;

    } else if (task->cmd == WAIT_CMD_START_CHARGE)
    {
          if (charger->present_status != MSG_CHARGER_READY)
          {
               task_err->errcode = charger->present_status;
                goto  commit;
          }
          if ( (charger->start_charge_order = (char *)malloc(sizeof(task->u.start_charge.order_num) )) == NULL)
          {
                task_err->errcode = EMSG_START_CHARGE_ERROR;
                goto  commit;
          }
          if ( (charger->start_charge_package = (char *)malloc(sizeof(task->u.start_charge.package))) == NULL)
          {
                free(charger->start_charge_order);
                task_err->errcode = EMSG_START_CHARGE_ERROR;
                goto  commit;
          }
          if ( (charger->uid = (char *)malloc(sizeof(task->u.start_charge.uid) )) == NULL)
          {
               free(charger->start_charge_order);
               free(charger->start_charge_package);
               task_err->errcode = EMSG_START_CHARGE_ERROR;
                goto  commit;
          }
          memcpy(charger->start_charge_order, task->u.start_charge.order_num, sizeof(task->u.start_charge.order_num) );
          memcpy(charger->start_charge_package, task->u.start_charge.package, sizeof(task->u.start_charge.package));
          memcpy(charger->uid, task->u.start_charge.uid, sizeof(task->u.start_charge.uid));
          charger->start_charge_energy = task->u.start_charge.energy;
          charger->start_charge_current = task->u.start_charge.current;
          charger->server_send_status = MSG_STATE_START_CHARGE;

    } else if (task->cmd == WAIT_CMD_STOP_CHARGE)
    {
          if (charger->present_status != MSG_CHARGER_CHARGING)
          {
               task_err->errcode = charger->present_status;
                goto  commit;
          }
          if ( (charger->uid = (char *)malloc(sizeof(task->u.start_charge.uid) )) == NULL)
          {
                task_err->errcode = EMSG_STOP_CHARGE_ERROR;
                goto  commit;
          }
          charger->stop_charge_username = task->u.stop_charge.username;
          memcpy(charger->uid, task->u.stop_charge.uid, sizeof(task->u.stop_charge.uid));
          charger->server_send_status = MSG_STATE_STOP_CHARGE;
          printf("stop  cmd ...\n");
    } else if (task->cmd == WAIT_CMD_CTRL)
    {
          if (charger->present_status != MSG_CHARGER_READY && charger->present_status != MSG_CHARGER_CHARGING)
          {
               task_err->errcode = charger->present_status;
                goto  commit;
          }
          charger->control_cmd =  task->u.control.value; 
          charger->server_send_status = MSG_STATE_CONTROL;
          printf("control  cmd...\n");
    }
    charger->last_update_time = time(0);
    charger->way = way;
    return 0;

commit:
      task_err->cmd = MSG_CMD_FROM_SERVER;
      task_err->cid = task->cid;
      printf("commit error...\n");
     if (commit_wrong)
         (*commit_wrong)(way, task_err);
    return -1;
}
void   check_charger_network(void)
{
        int i;

        for (i = 0; i < charger_manager.present_charger_cnt; i++)
        {
                        if (ChargerInfo[i].free_cnt_flag == 1 && ChargerInfo[i].free_cnt > 30)
                        {
                                ChargerInfo[i].free_cnt_flag = 0;
                                ChargerInfo[i].present_status = MSG_OFF_NET;
                                ev_uci_save_action(UCI_SAVE_OPT, true, "46", "chargerinfo.%s.PresentMode", ChargerInfo[i].tab_name);
                                // 断网情况下，清除电桩的一些状态，必需
                                if (ChargerInfo[i].is_charging_flag == 1)
                                {
                                     ChargerInfo[i].is_charging_flag = 0;
                                }
                        }
			if(ChargerInfo[i].free_cnt_flag == 1)
			 {
			        ChargerInfo[i].free_cnt++;
			 }
                        if (ChargerInfo[i].free_cnt_flag == 0)
                                ev_uci_save_action(UCI_SAVE_OPT, true, "46", "chargerinfo.%s.PresentMode", ChargerInfo[i].tab_name);
        }
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
    struct  wait_task  cmd, *pcmd;
    struct  join_wait_task       join_task;
    struct  upt     *update;
    CHARGER_INFO_TABLE  *charger;
    // 初始化chargerinfo命令
    if ( (pipefd = open(PIPEFILE, O_WRONLY, 0200)) < 0){
            exit(EXIT_FAILURE);
     }
    debug_msg("pthread of receive message is running");
   for ( ; ; )
   {
            // 阻塞住，每5秒超时
       recv_str = mqreceive_timed("/server.cmd", 100, 5);
       if (recv_str)
       {
                    debug_msg("队列有数据, 正在操作，str =%s...", recv_str);
#if 1
                    if ( (pcmd = parse_server_command(recv_str)) == NULL)
                    {
                            free(recv_str);
                            continue;
                    }
                    if ( check_have_all_handle(pipefd, SERVER_WAY, pcmd))
                    {
                            free(recv_str);
                            continue;
                    }
                    charger = look_for_charger_from_array(pcmd->cid);
                    if ( check_have_one_handle(SERVER_WAY, charger, pcmd, finish_task_add) < 0)
                    {
                         free(recv_str);
                         continue;
                    }
                    join_task.cid = pcmd->cid;
                    join_task.cmd = pcmd->cmd;
                    write(pipefd, &join_task, sizeof(struct join_wait_task));
                    free(recv_str);
                    continue;
#endif
          }
//         debug_msg("读取队列超时，准备读取UCI数据库, chargers:%d...", charger_manager.present_charger_cnt);

         check_charger_network();
         have_handle_flag = 0;
         if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.CID") < 0) //CMD 
            goto set_zero;
         CID_TMP = atoi(val_buff);
         if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.CMD") < 0) //CMD 
            goto set_zero;
         CMD_TMP = atoi(val_buff);
//       debug_msg("!CID = %d, !CID_TMP= %d, !CMD=%d, !CMD_TMP=%d\n", CID, CID_TMP, CMD, CMD_TMP); 

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
            if (CID == ChargerInfo[i].CID || CMD == WAIT_CMD_ALL_CHAOBIAO || CMD == WAIT_CMD_ALL_UPDATE)
            {
                have_handle_flag = 1;
                break;
            }
        }
        if (have_handle_flag != 1)
        {
            // 告诉后台
            log_msg("non match cid");
            goto set_zero;
        }
        
      // 封装命令
      cmd.cid = CID;
      cmd.cmd = CMD;
      pcmd = &cmd;
      switch (CMD)
      {
        case    WAIT_CMD_CHAOBIAO:
                    cmd.u.chaobiao.start_time = init_time;
                    cmd.u.chaobiao.end_time = time(0);//end_time;
                    debug_msg("pthread of receive command is CHAOBIAO");
        break;
        case    WAIT_CMD_CONFIG:
                    sprintf(val_buff, "%08d%c", CID, '\0');
                    strcpy(cmd.u.config.name, val_buff);
                    debug_msg("pthread of receive command is CONFIG");
                    printf("val_buff:%s\n", cmd.u.config.name);
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
                    memcpy(&cmd.u.update, update, sizeof(struct upt));
                    printf("pcmd->cid1:%d, pcmd->cmd:%d\n", pcmd->cid, pcmd->cmd);
                    if ( check_have_all_handle(pipefd, WEB_WAY, pcmd))
                            continue;
                    printf("pcmd->cid2:%d, pcmd->cmd:%d\n", pcmd->cid, pcmd->cmd);
                    charger = look_for_charger_from_array(pcmd->cid);
                    if ( check_have_one_handle(WEB_WAY, charger, pcmd, NULL) < 0)
                         continue;
                    printf("pcmd->cid3:%d, pcmd->cmd:%d\n", pcmd->cid, pcmd->cmd);
                    join_task.cid = pcmd->cid;
                    join_task.cmd = pcmd->cmd;
                    write(pipefd, &join_task, sizeof(struct join_wait_task));

                    debug_msg("pthread of service read update file = %s\n", val_buff);
        break;
        case    WAIT_CMD_ALL_CHAOBIAO:
                cmd.u.chaobiao.start_time = time(0) - 7 * 60 *60 *24;
                cmd.u.chaobiao.end_time = time(0);//end_time;
                check_have_all_handle(pipefd, WEB_WAY, pcmd);
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
                strncpy(cmd.u.start_charge.uid, val_buff, 32);
                printf("uid = %s\n", val_buff);
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.order") < 0) //CMD 
                {
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.uid");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.package");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.order");
                    goto set_zero;
                }
                memcpy(val_buff + strlen(val_buff), "                         ", 
                        sizeof(cmd.u.start_charge.order_num) - strlen(val_buff));
                strncpy(cmd.u.start_charge.order_num, val_buff, sizeof(cmd.u.start_charge.order_num));
                printf("order = %s\n", cmd.u.start_charge.order_num);
                 
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
                memcpy(val_buff + strlen(val_buff), "               ", sizeof(cmd.u.start_charge.package) - strlen(val_buff));
                strncpy(cmd.u.start_charge.package, val_buff, sizeof(cmd.u.start_charge.package));
                printf("package = %s\n", cmd.u.start_charge.package);
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.SERVER.energy") < 0) //CMD
                { 
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.uid");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.package");
	                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.order");
                    goto set_zero;
                }
                cmd.u.start_charge.energy = atoi(val_buff);
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
                strncpy(cmd.u.stop_charge.uid, val_buff, 32);
        break;
        default:
            goto set_zero;
        break;
      }
      // 加入等待队列
        ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.STATUS", "CLIENT"); //
        sprintf(val_buff, "%d%c", CMD_TMP, '\0');
	    ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CMD", "CLIENT");
        sprintf(val_buff, "%08d%c", CID, '\0');
	    ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CID", "CLIENT");

      debug_msg("pthread of service add task queue success");
      debug_msg("读取操作命令, CID[%d], CMD:%d ...", CID, CMD);
//      printf("=============================>加入链表命令操作成功\n");
set_zero:
	  ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.CMD", "SERVER");
       CMD = CMD_TMP = 0; 
   } //end for
    return NULL;
}

void *load_balance_pthread(void *arg)
{

}

//取出数据库配置信息，然后存入内存中
void  charger_info_init(int select)
{
	//查询数据库，获取表名数据
	char *tab_name = NULL, *tab_tmp = NULL;
	char name[20][10] = {{0}}, info[200] = {0};
        int  i = 0, j=0, charg_cnt = 0;
        int data, ii; 
        unsigned char tmp[2];

        if (select)
        {
	    // 存取充电记录变量
                memset(info, 0, sizeof(info));
                strcpy(info, "V1.01");
                ev_uci_save_action(UCI_SAVE_OPT, true, info, "chargerinfo.%s.chargerversion", "TABS"); 	
                sprintf(info, "%d", charger_manager.limit_max_current);
                ev_uci_save_action(UCI_SAVE_OPT, true, info, "chargerinfo.%s.maxcurrent", "TABS"); 	
                sprintf(info, "%d", charger_manager.total_num);
                ev_uci_save_action(UCI_SAVE_OPT, true, info, "chargerinfo.%s.chargernum", "TABS"); 
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.order");
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.package");    
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.energy");    
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.uid");    
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.CMD");
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.CID");
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.START_TIME");
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.SERVER.END_TIME");
                return ;
    }
//         printf("uci  init ... \n");
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
//	printf("charger name[%d] = %s\n",i,  name[i]);
//        printf("i+1 = %d\n", i+1);
	
        for(j =0; j <= i; j++)
	{
		if(ev_uci_data_get_val(info, sizeof(info), "chargerinfo.%s.CID", name[j]) < 0) //获取CID
		        exit(1);
                ChargerInfo[j].CID = atoi(info);
                if ( ev_uci_data_get_val(info, sizeof(info), "chargerinfo.%s.PresentMode", name[j]) < 0)
                     exit(1);
                if (atoi(info) == CHARGER_CHARGING)
                {
                        ChargerInfo[j].is_charging_flag = 1;
                        debug_msg("服务正在重启，有电桩正在充电");
                        if (ev_uci_data_get_val(info, sizeof(info), "chargerinfo.%s.privateID", name[j]) < 0) //
                                goto ret;
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
//                                debug_msg("%x ", data);
                                ChargerInfo[j].ev_linkid[ii] = data;     
                        }
                }
ret:
		if(ev_uci_data_get_val(info, sizeof(info), "chargerinfo.%s.Model", name[j]) < 0) // IP
		    exit(1);
		if(strncmp(info, "EVG-32N", 7) == 0)
		{
			ChargerInfo[j].model = EVG_32N;
		}
		else
		{
			ChargerInfo[j].model = EVG_16N;	
		}
//		ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.', "IP"); // IP
		//ChargerInfo[j].IP = inet_addr(info);
//		unsigned char* p = info;
//		char cnt;
//		for(cnt = 0; cnt < 4; cnt++)
//		{
//			if(cnt == 0){
//				str = strtok(p, ".");
//				if(str) 	ChargerInfo[j].IP[cnt] = atoi(str);
//				continue;
//			}
//			str = strtok(NULL, ".");
//			if(str)
//				ChargerInfo[j].IP[cnt] = atoi(str);
//		}	
#ifndef NDEBUG
//		printf("数据库取出的IP为------------------>%d.%d.%d.%d\n", ChargerInfo[j].IP[0], ChargerInfo[j].IP[1], ChargerInfo[j].IP[2], ChargerInfo[j].IP[3]);
#endif	
		// 赋值key值
		if(ev_uci_data_get_val(info, sizeof(info), "chargerinfo.%s.KEYB", name[j]) < 0)
		    exit(1);
		strncpy(ChargerInfo[j].KEYB, info, 16);
		if(ev_uci_data_get_val(info, sizeof(info), "chargerinfo.%s.ChargerType", name[j]) < 0)
		    exit(1);
                 ChargerInfo[j].charger_type = atoi(info);
                if(ev_uci_data_get_val(info, sizeof(info), "chargerinfo.%s.MAC", name[j]) < 0)
		    exit(1);
                strncpy(ChargerInfo[j].MAC, info, 17);
		strcpy(ChargerInfo[j].tab_name, name[j]);
		charg_cnt++;
		charger_manager.present_charger_cnt++;	// 全局数组，用于在内存中记录充电桩的个数,所有线程共享	
		printf("当前从数据库取出充电桩个数为--------------->%d\n", charger_manager.present_charger_cnt);
	} // end for

//	memcpy(Table_Name, tab_tmp, strlen(tab_tmp));	//赋值表名到全局数组
	free(tab_tmp);
	
    if ( ev_uci_data_get_val(info, sizeof(info), "chargerinfo.TABS.maxcurrent") >= 0)
                charger_manager.limit_max_current = atoi(info);
    else
                charger_manager.limit_max_current = 56;
    if ( ev_uci_data_get_val(info, sizeof(info), "chargerinfo.TABS.chargernum") >= 0)
                charger_manager.total_num = atoi(info);
    else
                charger_manager.total_num = 8;
    debug_msg("读取数据库电桩:");
    for (i = 0 ; i < charger_manager.present_charger_cnt; i++)
    {
        debug_msg("%s", ChargerInfo[i].tab_name);
    }
    return ;
}

