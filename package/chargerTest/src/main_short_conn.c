

//基于TCP的EV充电桩与EV_Router通信 服务器端
#include "./include/protocal_charger.h"
#include "./include/net_info.h"
#include "./include/serv_config.h"
#include "./include/err.h"
#include "./include/system_call.h"
#include "./include/CRC.h"
#include "./include/AES.h"
#include "./include/serial.h"
#include "./include/list.h"


// 定义出错信息结构
char    *err_string[] = {
    [0] = "Success",
//    [E_CHAOBIAO] = "ChaoBiao failed",
//    [E_CONFIG] = "Config file failed",
//    [E_ALL_UPDATE] = "Update all file failed",
//    [E_ONE_UPDATE] = "Update file failed"
};

pthread_mutex_t		clifd_mutex = PTHREAD_MUTEX_INITIALIZER;	// 线程互斥锁初始化
//pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;		// 线程条件变量初始化
pthread_mutex_t		serv_mutex = PTHREAD_MUTEX_INITIALIZER;	// 线程互斥锁初始化
pthread_cond_t		serv_cond = PTHREAD_COND_INITIALIZER;		// 线程条件变量初始化
pthread_rwlock_t	charger_rwlock;

// 充电桩信息，数组
CHARGER_INFO_TABLE	ChargerInfo[10] = {0};  	// 用于存入每台电桩信息	

unsigned char average_current_compare;
typedef struct {
    unsigned char   total_num;              //充电装总个数，默认给8
    unsigned char   limit_max_current;	//限制的最大的充电电流
    unsigned char   present_charger_cnt;	//当前
    unsigned char   present_off_net_cnt;	//当前断网的个数
    unsigned char   present_networking_cnt;	//当前联网的总数
    unsigned char   present_charging_cnt;	//当前正在充电的个数
    int             have_powerbar_serial_fd;            //打开与灯板通信的串口描述符
    int             timeout_cnt;
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
int  charger_serv(const int fd, const int cmd, CHARGER_INFO_TABLE *charger,  BUFF *bf, int *index);
void protocal_init_head(unsigned char CMD,  char *p_str, unsigned int cid);
int  sock_serv_init(void);
void * thr_fn(void * arg);
void daemonize(const char *cmd);
void msleep(unsigned long mSec);
int already_running(void);
void print_PresentMode(unsigned char pmode);
void print_SubMode(unsigned short submode);
void print_SelectSocket(unsigned char socket);
void key_init(unsigned char *key);
void  charger_info_init(int select);
void *pthread_listen_program(void * arg);
void *thread_main(void * arg);
void *pthread_service_send(void *arg);
void *pthread_service_receive(void *arg);
void *sigle_pthread(void *arg);
void *load_balance_pthread(void *arg);
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
    sprintf(name, "%s/%s%c", WORK_DIR, EXCEPTION_DIR, '\0');
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
#define MAX_LEN     1500
void * thread_main(void * arg)
{
	int connfd, n, charger_index = -1, Error = 0, cmd, return_val;
    BUFF    bf = {NULL, NULL, NULL, 0};
        
	connfd = ((int)arg);
	
    if(pthread_detach(pthread_self()) < 0)
	{
        err_sys("pthread detach failed");
        return NULL;
	}
    if ( (bf.recv_buff = (char *)malloc(MAX_LEN)) == NULL)
        goto exitt;
    if ( (bf.send_buff = (char *)malloc(MAX_LEN)) == NULL)
        goto exitt;
    if ( (bf.val_buff = (char *)malloc(MAX_LEN)) == NULL)
        goto exitt;

    memset(bf.recv_buff, 0, MAX_LEN); 
    memset(bf.send_buff, 0, MAX_LEN); 
    memset(bf.val_buff, 0, MAX_LEN); 
	// 接收电桩的数据
    // 接收超时
    if (readable_timeout(connfd, 20) == 0)
    {
        debug_msg("server read timeout ...\n");
        goto exitt;
    }

    if ( (n = read(connfd, bf.recv_buff, MAX_LEN)) <= 0)
    {
        if (charger_index != -1)
         {
             debug_msg("client exit, CID[%d]", charger->CID);
             printf("client exit, CID[%d]\n", charger->CID);
         } else
         {
             debug_msg("charger is  exit ....");
             printf("charger is  exit ....");
         }
        goto exitt; 
    }
    // 查找
    for (i = 0; i < charger_manager.present_charger_cnt; i++)
    {
        // CID 比较
        if (memcmp(&ChargerInfo[i].CID, bf->recv_buff + 5, 4) == 0)
        {
            charger_index = i;
            break;
        }
    }
    // 检查有没有命令需要处理
    if (charger_index != -1)
    {
         bf.recv_cnt = n;
         have_wait_command(&ChargerInfo[charger_index], &bf);
         cmd = ChargerInfo[charger_index].present_cmd;
    } else
    {
         cmd = bf.recv_buff[4];
         bf.recv_cnt = n;
    }
    // 开始服务
    if (charger_index != -1)
    {
	    if ( (return_val = charger_serv(connfd, cmd, &ChargerInfo[charger_index], &bf, &charger_index)) < 0)
             goto error;  
   } else
   {
	   if ( (return_val = charger_serv(connfd, cmd, NULL, &bf, &charger_index)) < 0)
             goto error;  
   }
//        error_hander(return_val, &ChargerInfo[charger_index], &bf, bf.ErrorCode);
//       continue;c

       // 发送错误信息给客户端
error:  
    if (charger_index != -1)  
       error_hander(return_val, &ChargerInfo[charger_index], &bf, bf.ErrorCode);
 exitt:
	// 关闭连接
    if (bf.recv_buff != NULL)
        free(bf.recv_buff);
    if (bf.send_buff != NULL)
        free(bf.send_buff);
    if (bf.val_buff != NULL)
        free(bf.val_buff);
	shutdown(connfd, SHUT_RDWR);  
	close(connfd);
	pthread_exit((void *)0);
//	sock_close(connfd);
	return NULL;
}


// 充电协议处理程序
int  
charger_serv(const int fd, const int cmd, CHARGER_INFO_TABLE *charger,  BUFF *bf, int *index)
{
    char    *key_addr = NULL, *sptr = NULL, CID_flag = 0, ChargerCnt;
	unsigned int             len, i, n, ops, err, tmp_4_val, declen = 0, enclen = 0, CID = 0;
	unsigned short	CRC = 0x1d0f, SubMode = 0, tmp_2_val = 0;
    time_t   tm;
    FILE    *file;
    
    // 协议初始化
	
    CID = *(unsigned int *)(bf->recv_buff+5);
	printf("#######------->CMD =%#x, CID = %d, CNT=%d, index=%d\n", cmd, *(unsigned int *)(bf->recv_buff + 5), charger_manager.present_charger_cnt, *index);

    // 1.数据解密,CRC判断，帧头判断略
	if( cmd !=  0x10)// 找到匹配的
	{
        if (*index == -1)  // 服务器重启会卡住在这里退出，但是之后发送0x10后，恢复正常。
        {
            bf->ErrorCode = ESERVER_RESTART;
            return -1;
        }
		charger->free_cnt = 0;
		charger->free_cnt_flag = 1;
		My_AES_CBC_Decrypt(charger->KEYB, bf->recv_buff+9, bf->recv_cnt - 9, bf->send_buff);
	}else
	{
		My_AES_CBC_Decrypt(KEYA, bf->recv_buff+9, bf->recv_cnt - 9, bf->send_buff);
	}
	declen = bf->recv_cnt - 9;
	len = 0;
	RePadding(bf->send_buff, declen, bf->recv_buff+9, &len);
	len +=9;
	CRC = getCRC(bf->recv_buff, len-2);
	if(CRC !=  *(unsigned short *)(bf->recv_buff+len-2))
	{
		debug_msg("CRC校验失败:CID[%d] ...", *(unsigned int *)(bf->recv_buff + 5));
        bf->ErrorCode = ESERVER_CRC_ERR;
		//出错处理
		return -1;
	}
    msleep(100);
    // 判断有没有抄表指令
//	bzero(bf->send_buff, strlen(bf->send_buff));
	// 电桩协议逻辑
 
    switch (cmd) {
		case	CHARGER_CMD_CONNECT: // 0x10  连接请求

			// 用读的方式，锁住读写锁，然后查看信息,未实现
			printf("连接请求...\n");
            // 更新 index 变量
            ChargerCnt = charger_manager.present_charger_cnt;
	        for(i=0; i<ChargerCnt; i++)
	        {
		        if( memcmp(&ChargerInfo[i].CID, &CID, 4) == 0) // 判断CID是否相同
		        {
			        printf("找到的CID========>%d\n", ChargerInfo[i].CID);
				    *index = i;
				    CID_flag = 1;
				    break;	
				    //continue;
		        }
	        }
            unsigned char tab_buff[20] = {0};
			if( (key_addr = (char *)malloc(16)) == NULL )
				goto cmd_0x10;
            //获取随机KEYB值
			key_init(key_addr);	
			
            // 存在的电桩
            if (CID_flag == 1)  
			{
				//将改变的IP存入数据库
				sprintf(bf->val_buff, "%d.%d.%d.%d", bf->recv_buff[9], bf->recv_buff[10], bf->recv_buff[11], bf->recv_buff[12]);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.IP", ChargerInfo[(*index)].tab_name) < 0)	// 存入IP
                    goto cmd_0x10;
                printf("aaaa\n");
				memcpy(bf->val_buff, key_addr, 16);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.KEYB", ChargerInfo[(*index)].tab_name) < 0)
                    goto cmd_0x10;
				strncpy(bf->val_buff, bf->recv_buff+18, 10);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Model", ChargerInfo[(*index)].tab_name) < 0) // series
                    goto cmd_0x10;
				strncpy(bf->val_buff, bf->recv_buff+28, 10);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Series", ChargerInfo[(*index)].tab_name) < 0)
                    goto cmd_0x10;
                sprintf(bf->val_buff, "%d", bf->recv_buff[42]);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerType", tab_buff) < 0)	
                    goto cmd_0x10;
				sprintf(bf->val_buff, "%d.%02d%c", bf->recv_buff[41], bf->recv_buff[40], '\0');	//ChargerVersion
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerVersion", ChargerInfo[(*index)].tab_name) < 0)
                    goto cmd_0x10;
				memcpy(ChargerInfo[(*index)].KEYB, key_addr, 16);
                ChargerInfo[(*index)].wait_cmd = WAIT_CMD_NONE;
				strncpy(bf->val_buff, bf->recv_buff+18, 10);
				if(!strcmp(bf->val_buff, "EVG-16N"))
				{
					ChargerInfo[(*index)].model = EVG_16N;
				}else if(!strcmp(bf->val_buff, "EVG-32N"))
				{
					ChargerInfo[(*index)].model = EVG_32N;	
				}else if(!strcmp(bf->val_buff, "EVG-32NW"))
				{
					ChargerInfo[(*index)].model = EVG_32N;	
				}else if(!strcmp(bf->val_buff, "EVG-32N"))
				{
					ChargerInfo[(*index)].model = EVG_32N;	
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
                sprintf(mac_addr, "%2x:%2x:%2x:%2x:%2x:%2x%c", bf->recv_buff[42], bf->recv_buff[43], bf->recv_buff[44], bf->recv_buff[45], bf->recv_buff[46], bf->recv_buff[47], '\0');
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
                            sprintf(bf->val_buff, "%d%c", CID, '\0');
                            debug_msg("===============================================同一个mac地址:%s", mac_addr);
				            if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.CID", ChargerInfo[i].tab_name) < 0)
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
                    offset = charger_manager.present_charger_cnt;
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
                    debug_msg("offset = %d\n", offset);
                 }   
				// 保存CID，IP， KEYD到数据库
				sprintf(bf->val_buff, "%d", CID);
				printf("tab_buff ===%s\n", tab_buff);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, mac_addr, "chargerinfo.%s.MAC", tab_buff )  < 0)
			        goto cmd_0x10;
				if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.CID", tab_buff )  < 0)
					goto cmd_0x10;
				sprintf(bf->val_buff, "%d.%d.%d.%d\0", bf->recv_buff[9], bf->recv_buff[10], bf->recv_buff[11], bf->recv_buff[12]);	// IP
				if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.IP", tab_buff )  < 0)
					goto cmd_0x10;
				memcpy(bf->val_buff, key_addr, 16);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.KEYB", tab_buff)  < 0)
					goto cmd_0x10;
				strncpy(bf->val_buff, bf->recv_buff+18, 10);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Model", tab_buff) < 0) // series
					goto cmd_0x10;
				strncpy(bf->val_buff, bf->recv_buff+28, 10);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Series", tab_buff) < 0)	
					goto cmd_0x10;
				sprintf(bf->val_buff, "%d.%02d", bf->recv_buff[41], bf->recv_buff[40]);	//chargerVersion
				if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerVersion", tab_buff) < 0)	
					goto cmd_0x10;
                sprintf(bf->val_buff, "%d", bf->recv_buff[42]);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerType", tab_buff) < 0)	
					goto cmd_0x10;
				ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.SelectCurrent", tab_buff);	
				ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.PresentOutputCurrent", tab_buff);
				// 写入数据库
				ChargerInfo[offset].CID = CID;
                ChargerInfo[offset].charger_type = bf->recv_buff[42];
                ChargerInfo[offset].wait_cmd = WAIT_CMD_NONE;
                strcpy(ChargerInfo[offset].MAC, mac_addr);
				memcpy(ChargerInfo[offset].KEYB, key_addr, 16);
				strcpy(ChargerInfo[offset].tab_name, tab_buff);
				strncpy(bf->val_buff, bf->recv_buff+18, 10);
                *index = offset;
				if(!strcmp(bf->val_buff, "EVG-16N"))
				{
					ChargerInfo[offset].model = EVG_16N;
				}else if(!strcmp(bf->val_buff, "EVG-32N"))
				{
					ChargerInfo[offset].model = EVG_32N;	
				}else if(!strcmp(bf->val_buff, "EVG-32NW"))
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
            printf("a...\n");
             if ( gernal_command(fd, CHARGER_CMD_CONNECT_R, &ChargerInfo[(*index)], bf) < 0)
              {
                  goto cmd_0x10;
              }
            bf->ErrorCode = ESERVER_SEND_SUCCESS;
            free(sptr);
            free(key_addr);
            return 0;
cmd_0x10:
            if (CID_flag!= 1)
            {
		        if(pthread_rwlock_unlock(&charger_rwlock) < 0){
                     debug_msg("pthrea_rwlock_unlock failed");
                    exit(1);
                }
            }
            bf->ErrorCode = ESERVER_API_ERR;
            if (sptr != NULL)
              free(sptr);
            if (key_addr != NULL)
               free(key_addr);
		    return -1;	
           // 发送回应
		break;

		case 	CHARGER_CMD_HB:	 // 0x34  心跳
			printf("心跳请求...\n");
                //解决充电完成，还未解锁导致浮点计算出错的BUG
//			if(bf->recv_buff[9] ==  CHARGER_CHARGING_COMPLETE_LOCK && ChargerInfo[(*index)].is_charging_flag == 0) 
//			{
//				ChargerInfo[(*index)].is_charging_flag = 1;
//			} 
			sprintf(bf->val_buff, "%d%c", bf->recv_buff[9], '\0');	//presentMode
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentMode", charger->tab_name);
			ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.PresentOutputCurrent", charger->tab_name);
			ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.SelectCurrent", charger->tab_name);
			sprintf(bf->val_buff, "%d%c", SubMode, '\0'); // SUB_MODE
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.SubMode", charger->tab_name);
			// 心跳处理略
			//  回复心跳
		    if (gernal_command(fd, CHARGER_CMD_HB_R, charger, bf) < 0)
            {
                bf->ErrorCode = ESERVER_API_ERR;
                return -1;
            }
            bf->ErrorCode = ESERVER_SEND_SUCCESS;
            return 0;
		break;

		case	CHARGER_CMD_CHARGE_REQ:	// 0X54 充电请求
			printf("充电请求...\n");
            charger->real_current = 0;
			charger->is_charging_flag = 0;
            charger_manager.timeout_cnt = 0;
			// 统计充电请求个数
			//记录客户信息
			ChargerInfo[(*index)].start_time = time(0);	//开始充电时间
			ChargerInfo[(*index)].charging_code = *(unsigned short *)(bf->recv_buff+33);			//赋值charging_code
			// load  balance 处理
//			if( (bf->recv_buff[29] !=  0 )&& (bf->recv_buff[29]%16 == 0))
//			{
//				ChargerInfo[(*index)].model = bf->recv_buff[29];
//			}
			ChargerInfo[(*index)].real_current = 10;
            
#if 0
            while(ChargerInfo[(*index)].real_current <= 0 && charger_manager.timeout_cnt <100)
		    {
				printf(".....................%d...................正在睡眠等待分配电流\n", ChargerInfo[(*index)].real_current);
				msleep(300);
                charger_manager.timeout_cnt++;
//					return 0;
			}
            if(charger_manager.timeout_cnt >= 100 && ChargerInfo[(*index)].real_current <= 0)
            {
                    // failure
                   pthread_mutex_unlock(&serv_mutex);
                   return 0;
            }
            ChargerInfo[(*index)].is_charging_flag = 1;
#endif            
        // 发送数据到后台,上锁
	    
        pthread_mutex_lock(&serv_mutex);
        sprintf(bf->val_buff, "/Charging/canStartCharging?");
        sprintf(bf->val_buff+strlen(bf->val_buff), "chargerID=%08d'&'", CID);
        for (i = 0; i < 16; i++)
        {
            sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[36 + i]);
        }
        
        sprintf(bf->val_buff + strlen(bf->val_buff), "privateID=%s", bf->send_buff);
        memcpy(ChargerInfo[(*index)].ev_linkid, bf->recv_buff+36, 16);
        // 发送
        cmd_frun("dashboard url_post 10.9.8.2:8080/ChargerAPI %s", bf->val_buff);
        sptr = mqreceive_timed("/dashboard.checkin");
        if (!sptr)
        {
            ChargerInfo[(*index)].is_charging_flag = 0;
		    pthread_mutex_unlock(&serv_mutex);
		    charger->system_message = 0x02;
		    //tell charger not to wait
		    goto reply_to_charger;
        }
        printf("=================================================>str = %s\n", sptr);
        switch ( *sptr-48) 
        {
            case    SYM_Charging_Is_Starting: 
                    debug_msg("后台允许充电...");
                    ev_uci_save_action(UCI_SAVE_OPT, true, bf->send_buff,
				       "chargerinfo.%s.privateID", ChargerInfo[(*index)].tab_name);
			        charger->system_message = 0x01;			// systemMessage
            break;
            case    SYM_Charging_Is_Stopping:
                    debug_msg("后台不允许充电...");
			        charger->system_message = 0x02;			// systemMessage
            break;
            case    SYM_No_Access_Right:
                    debug_msg("没有权限...");
			        charger->system_message = 0x03;			// systemMessage
            break;
            case    SYM_Not_Money_Charging:
                    debug_msg("余额不足");
                    charger->system_message = 0x04;
            break;
            case    SYM_EV_Link_Not_Valid:
                    debug_msg("易冲卡无效");
                    charger->system_message = 0x06;
            break;

            case   SYM_EV_Link_Is_Used:
                    debug_msg("易冲卡已经使用");
                    charger->system_message = 0x07;
            break;
            default:
                   charger->system_message = 0x02;
            break;
        }           
       
        free(sptr);
        if (charger->system_message ==  SYM_Charging_Is_Starting)
        {
            // 向后台发送充电
            sptr = NULL;
            cmd_frun("dashboard url_post 10.9.8.2:8080/ChargerAPI %s", bf->val_buff);
            sptr = mqreceive_timed("/dashboard.checkin");
            if (sptr != NULL)
                free(sptr);
            charger->target_mode = CHARGER_CHARGING;
        } else
        {
            charger->target_mode = CHARGER_READY;
            charger->is_charging_flag = 0;
            pthread_mutex_unlock(&serv_mutex);
		    goto reply_to_charger;
        }
        pthread_mutex_unlock(&serv_mutex);
#if 1
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->send_buff, "chargerinfo.%s.privateID", charger->tab_name);
			sprintf(bf->val_buff, "%d%c", bf->recv_buff[17], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentMode", charger->tab_name);
			tmp_2_val = *(unsigned short *)(bf->recv_buff + 18);
			sprintf(bf->val_buff, "%d%c", tmp_2_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.SubMode", charger->tab_name);
			sprintf(bf->val_buff, "%d%c", bf->recv_buff[29], '\0');	//select current
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.SelectCurrent", charger->tab_name);
			sprintf(bf->val_buff, "%d%c", *(unsigned short *)(bf->recv_buff + 30), '\0');	//presentoutputcurrent
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentOutputCurrent", charger->tab_name);
			tmp_2_val = *(unsigned short *)(bf->recv_buff+34);
			sprintf(bf->val_buff, "%d%c", tmp_2_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargingCode", charger->tab_name);
			sprintf(bf->val_buff, "%d%c", bf->recv_buff[56], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerWay", charger->tab_name);
#endif
reply_to_charger:			
            if (gernal_command(fd, CHARGER_CMD_CHARGE_REQ_R, charger, bf) < 0)
            {
                bf->ErrorCode = ESERVER_API_ERR;
                return -1;
            }
            bf->ErrorCode = ESERVER_SEND_SUCCESS;
            return 0;

		case	CHARGER_CMD_STATE_UPDATE:	// 0X56 充电中状态更新
			printf("充电中状态更新...\n");
            ChargerInfo[(*index)].is_charging_flag = 1;
            ChargerInfo[(*index)].real_time_current = *(unsigned short *)(bf->recv_buff + 23);
			if(bf->recv_buff[17] == CHARGER_CHARGING && ChargerInfo[(*index)].is_charging_flag == 0)  //解决服务器短暂断开重新连接
			{
				ChargerInfo[(*index)].is_charging_flag = 1;
			}
			printf("==========================================> bf->recv_buff[34] = %d\n", bf->recv_buff[34]);
			tmp_2_val = *(unsigned short *)(bf->recv_buff+32);
#if 1
            sprintf(bf->val_buff, "%d%c", bf->recv_buff[17], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentMode", charger->tab_name);
			tmp_2_val = (bf->recv_buff[23] << 8 | bf->recv_buff[24]);
            sprintf(bf->val_buff, "%d%c", tmp_2_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentOutputCurrent", charger->tab_name);
			tmp_2_val = *(unsigned short *)(bf->recv_buff+34);
			sprintf(bf->val_buff, "%d%c", tmp_2_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Duration", charger->tab_name);
			tmp_4_val = *(unsigned int *)(bf->recv_buff + 30);
			sprintf(bf->val_buff, "%d%c", tmp_4_val, '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Power", charger->tab_name);
			sprintf(bf->val_buff, "%d%c", bf->recv_buff[51], '\0');
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerWay", charger->tab_name);
            bf->val_buff[0] = '\0';
            for (i = 0; i < 16; i++)
            {
                sprintf(bf->val_buff + strlen(bf->val_buff), "%02x", bf->recv_buff[36 + i]);
            }
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.privateID", charger->tab_name);
            // 快冲
            if (charger->charger_type == 2 || charger->charger_type == 4)
            {
                sprintf(bf->val_buff, "%d", bf->recv_buff[27]);
			    ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Soc", charger->tab_name);
                tmp_2_val = (bf->val_buff[28] << 8 | bf->val_buff[29]);
                sprintf(bf->val_buff, "%d", tmp_2_val);
			    ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Tilltime", charger->tab_name);
            }

#endif
            charger->target_mode = CHARGER_CHARGING;
            charger->system_message = 0x01;
            // 回应心跳
            if (gernal_command(fd, CHARGER_CMD_HB_R, charger, bf) < 0)
            {
                bf->ErrorCode = ESERVER_API_ERR;
                return -1;
            }
            bf->ErrorCode = ESERVER_SEND_SUCCESS;
            return 0;
		break;

		case	CHARGER_CMD_STOP_REQ:	// 0x58  停止充电请求
			debug_msg("停止充电请求, CID[%d] ...", charger->CID);
			ChargerInfo[(*index)].end_time = time(0);				//获取充电结束时间
			ChargerInfo[(*index)].power = *(unsigned int *)(bf->recv_buff+39);	//获取结束充电的电量
			ChargerInfo[(*index)].is_charging_flag = 0;
			ChargerInfo[(*index)].real_current = 0;

			// 发送数据给后台
			sprintf(bf->val_buff, "/ChargerState/stopState?");
			sprintf(bf->val_buff + strlen(bf->val_buff), "key={chargers:[{chargerId:\\\"%08d\\\",", CID);
            for (i = 0; i < 16; i++) {
                sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[45 + i]);
            }
            sprintf(bf->val_buff + strlen(bf->val_buff), "privateID:\\\"%s\\\",", bf->send_buff);
			tmp_4_val = *(unsigned int *)(bf->recv_buff + 39);
			sprintf(bf->val_buff + strlen(bf->val_buff), "power:%d,", tmp_4_val);
			tmp_2_val =(bf->recv_buff[62] << 8 | bf->recv_buff[63]);
			sprintf(bf->val_buff + strlen(bf->val_buff), "chargingRecord:%d,", tmp_2_val);
			sprintf(bf->val_buff + strlen(bf->val_buff), "mac:\\\"%s\\\",", ChargerInfo[(*index)].MAC);
			sprintf(bf->val_buff + strlen(bf->val_buff), "chargingType:%d,", bf->recv_buff[61]);
			sprintf(bf->val_buff + strlen(bf->val_buff), "status:%d}]}", bf->recv_buff[17]);
			debug_msg("len: %d", strlen(bf->val_buff));
			cmd_frun("dashboard url_post 10.9.8.2:8080/ChargerAPI %s", bf->val_buff);
		
			memset(bf->val_buff, 0, strlen(bf->val_buff));
			ev_uci_delete( "chargerinfo.%s.privateID", charger->tab_name);
			ev_uci_delete( "chargerinfo.%s.ChargingCode", charger->tab_name);

            if (charger->start_time != 0)
            {
			    sprintf(bf->val_buff, "%08d,%ld,%ld,%d,%d,",  CID, charger->start_time, charger->end_time, charger->power,bf->recv_buff[61]);
                for (i = 0; i < 16; i++)
                {
                    sprintf(bf->val_buff + strlen(bf->val_buff), "%02x", bf->recv_buff[45 + i]);
                }
                bf->val_buff[strlen(bf->val_buff)] = '\n';
                sprintf(bf->send_buff, "%s/%s/%08d%c", WORK_DIR, RECORD_DIR, charger->CID, '\0');
                if ( (file = fopen(bf->send_buff, "ab+")) == NULL)
                     goto replay_0x58;
                fwrite(bf->val_buff, 1, strlen(bf->val_buff), file);
                fclose(file);
            }
replay_0x58:
            sprintf(bf->val_buff, "%d%c", bf->recv_buff[17], '\0');
			//写入数据库，更新相应表信息
			//当前模式
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentMode", charger->tab_name);
//			tmp_2_val = *(unsigned short*)(bf->recv_buff+18);
//			sprintf(bf->val_buff, "%d", tmp_2_val);
//			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.SubMode", ChargerInfo[(*index)].tab_name);
//			sprintf(bf->val_buff, "%d", bf->recv_buff[29]);
//			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.SelectCurrent", ChargerInfo[(*index)].tab_name);
			tmp_2_val = *(unsigned short*)(bf->recv_buff+33);
			sprintf(bf->val_buff, "%d", tmp_2_val);
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargingCode", charger->tab_name);
            tmp_2_val = (bf->recv_buff[32] >> 8 | bf->recv_buff[33]);
            sprintf(bf->val_buff, "%d", tmp_2_val);
			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentOutputCurrent", charger->tab_name);

             // 回应
            charger->target_mode = CHARGER_READY;
            charger->system_message = SYM_Charging_Is_Stopping;
            if (gernal_command(fd, CHARGER_CMD_STOP_REQ_R, charger, bf) < 0)
            {
                bf->ErrorCode = ESERVER_API_ERR;
                return -1;
            }
            bf->ErrorCode = ESERVER_SEND_SUCCESS;
            return 0;
		break;

		case 	CHARGER_CMD_CONFIG:  // 0x50   回应推送配置信息
			debug_msg("接收配置文件请求, CID[%d] ...\n", charger->CID);

            charger->config_num = bf->recv_buff[9] + 1;
            // 推送完成
            if (bf->recv_buff[10] == 1) 
            {
                bf->ErrorCode = ECONFIG_FINISH;
                return 0;
            }

            if (gernal_command(fd, CHARGER_CMD_CONFIG_R, charger, bf) < 0)
            {
                bf->ErrorCode = ECONFIG_API_ERR;
                return -1;
            }
            bf->ErrorCode = ESERVER_SEND_SUCCESS;
			printf("回应配置文件请求成功...\n");
            return 0;
		break;

        case    CHARGER_CMD_CONFIG_R: //发送配送配置命令
                
                debug_msg("正在发送推送配置命令,CID[%#x] ...", charger->CID);
                charger->config_num = 0;
                if (gernal_command(fd, CHARGER_CMD_CONFIG_R, charger, bf) < 0)
                {
                    bf->ErrorCode = ECONFIG_API_ERR;
                    return -1;
                }
                bf->ErrorCode = ESERVER_SEND_SUCCESS;
                return 0;
        break;

        case    CHARGER_CMD_YUYUE_R:
                debug_msg("正在发送预约命令,CID[%#x] ...", charger->CID);
                if (gernal_command(fd, CHARGER_CMD_YUYUE_R, charger, bf) < 0)
                {
                    bf->ErrorCode = EYUYUE_API_ERR;
                    return -1;
                }
                bf->ErrorCode = ESERVER_SEND_SUCCESS;
                return 0;
        break;

		case	CHARGER_CMD_CTRL_R:	//控制电桩回应
                debug_msg("正在发送控制命令,CID[%#x] ...", charger->CID);
                if (gernal_command(fd, CHARGER_CMD_CTRL_R, charger, bf) < 0)
                {
                    bf->ErrorCode = ECONTROL_API_ERR;
                    return -1;
                }
                bf->ErrorCode = ESERVER_SEND_SUCCESS;
                return 0;
		break;
        case  CHARGER_CMD_CTRL:
                debug_msg("接收到控制命令,CID[%#x] ...", charger->CID);
                if (bf->recv_buff[13] == 1)
                {
                    bf->ErrorCode = ECONTROL_FINISH;
                    return 0;
                } else
                {
                    bf->ErrorCode = ECONTROL_NO_CTRL;
                    return 0;
                }
        break;

        case    CHARGER_CMD_CHAOBIAO_R:  // 抄表发送
                debug_msg("正在发送抄表请求 ...");        
                charger->cb_target_id  = 0;
                charger->cb_charging_code  = 0;
                if (gernal_command(fd, CHARGER_CMD_CHAOBIAO_R, charger, bf) < 0)
                {
                    bf->ErrorCode = ECHAOBIAO_API_ERR;
                    return -1;
                 }
                 bf->ErrorCode = ESERVER_SEND_SUCCESS;
                return 0;
        break;

		case	CHARGER_CMD_CHAOBIAO:	//抄表回应
                printf("接收到抄表请求...\n");
                debug_msg("接收到抄表记录:CID[%d], total = %d, present_cnt:%d, complete:%d", \
                        charger->CID, *(unsigned short *)(bf->recv_buff + 984), bf->recv_buff[981], bf->recv_buff[986]);

                if (bf->recv_buff[986] == 0)
                {
                        charger->cb_charging_code = (*(unsigned short *)(bf->recv_buff + 982)) - 15;
                        charger->cb_target_id = 0;
                } else
                {
                    charger->cb_charging_code =  0;
                    charger->cb_target_id = 0xFFFF;
                    
                }
                //写充电记录到文件
                if(bf->recv_buff[981] > 0 && write(charger->file_fd, bf->recv_buff + 21, 64*bf->recv_buff[981]) != 64*bf->recv_buff[981])
                {
                    debug_msg("write chaobiao failed");
                    bf->ErrorCode = ECHAOBIAO_API_ERR;
                    return -1;
                }
                if (gernal_command(fd, CHARGER_CMD_CHAOBIAO_R, charger, bf) < 0)
                {
                    if (bf->recv_buff[986] != 1)
                        bf->ErrorCode = ECHAOBIAO_API_ERR;
                    else 
                    {
                        bf->ErrorCode = ECHAOBIAO_FINISH;
                        return 0;
                    }
                    return -1;
                }
                if (bf->recv_buff[986] == 1)
                    bf->ErrorCode = ECHAOBIAO_FINISH;
                 else
                    bf->ErrorCode = ESERVER_SEND_SUCCESS;
                printf("回应抄表请求成功...\n"); 
                return 0;
        break;

        case    CHARGER_CMD_START_UPDATE_R:
                 debug_msg("正在发送更新请求:CID[%#x]", charger->CID);
                 if (gernal_command(fd, CHARGER_CMD_START_UPDATE_R, charger, bf) < 0) 
                 {
                        bf->ErrorCode = EUPDATE_API_ERR;
                        return -1;
                  }
                 bf->ErrorCode = ESERVER_SEND_SUCCESS;
                 return 0;
        break;
		
        case	CHARGER_CMD_UPDATE:	// 0x95 更新软件回应	
			debug_msg("接收更新软件请求,CID[%d], package:%d\n", charger->CID, bf->recv_buff[11]);
			if(bf->recv_buff[12] == 1)
            {            // 线程不安全，未作处理
                bf->ErrorCode = EUPDATE_FINISH; 
                return 0;
            }
            if ( gernal_command(fd, CHARGER_CMD_UPDATE_R, charger, bf) < 0)
            {
                bf->ErrorCode = EUPDATE_API_ERR;
                return -1;
            }
            bf->ErrorCode = ESERVER_SEND_SUCCESS;
            printf("回应更新软件回应成功...\n");
            return 0;;
		break;
        case    CHARGER_CMD_EXCEPTION:
                debug_msg("电桩异常 CID[%#x] ...", charger->CID);
                struct tm *tim = (struct tm *)malloc(sizeof(struct tm));
                if (tim == NULL)
                {
                    bf->ErrorCode = ESERVER_API_ERR;
                    return -1;
                }
                tm = *(time_t *)(bf->recv_buff + 9);
                if (localtime_r(&tm, tim) == NULL)
                {
                    bf->ErrorCode = ESERVER_API_ERR;
                    free(tim);
                    return -1;
                }
                sprintf(bf->val_buff, "[%d-%d-%d %d:%d:%d]", tim->tm_year+1900, tim->tm_mon+1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);
                sprintf(bf->val_buff + strlen(bf->val_buff), "CID=%08x:", charger->CID);
                sprintf(bf->val_buff + strlen(bf->val_buff), "presentMode=%d:", bf->recv_buff[13]);
                tmp_4_val = *(unsigned int *)(bf->recv_buff +16);
                sprintf(bf->val_buff + strlen(bf->val_buff), "AccPowr=%d:", tmp_4_val);
                tmp_2_val = *(unsigned short*)(bf->recv_buff + 20);
                sprintf(bf->val_buff + strlen(bf->val_buff), "AChargerFre=%d:", tmp_2_val);
                sprintf(bf->val_buff + strlen(bf->val_buff), "Socket=%d:", bf->recv_buff[21]);
                tmp_2_val = *(unsigned short*)(bf->recv_buff + 22);
                sprintf(bf->val_buff + strlen(bf->val_buff), "current=%d:", tmp_2_val);
                tmp_2_val = *(unsigned short*)(bf->recv_buff + 24);
                sprintf(bf->val_buff + strlen(bf->val_buff), "voltage=%d:", tmp_2_val);
                sprintf(bf->val_buff + strlen(bf->val_buff), "soc=%d:", bf->recv_buff[26]);
                tmp_2_val = *(unsigned short*)(bf->val_buff + 27);
                sprintf(bf->val_buff + strlen(bf->val_buff), "tilltime=%d:", tmp_2_val);
                tmp_4_val = *(unsigned int *)(bf->recv_buff + 29);
                sprintf(bf->val_buff + strlen(bf->val_buff), "power=%d:", tmp_4_val);
                tmp_2_val = *(unsigned short *)(bf->recv_buff + 33);
                sprintf(bf->val_buff + strlen(bf->val_buff), "usetime=%d:", tmp_2_val);
                bf->send_buff[0] = '\0';
                for (i = 0; i < 16; i++)
                {
                    sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[35 + i]);
                }
                sprintf(bf->val_buff + strlen(bf->val_buff), "evlinkid=%s:", bf->send_buff);
                sprintf(bf->val_buff + strlen(bf->val_buff), "chargerway=%d:", bf->recv_buff[51]);
                tmp_2_val = *(unsigned short *)(bf->recv_buff + 52);
                sprintf(bf->val_buff + strlen(bf->val_buff), "chargercode=%d:", tmp_2_val);
                tmp_4_val = *(unsigned int *)(bf->recv_buff + 54);
                sprintf(bf->val_buff + strlen(bf->val_buff), "errcode1=%d:", tmp_4_val);
                tmp_4_val = *(unsigned int *)(bf->recv_buff + 58);
                sprintf(bf->val_buff + strlen(bf->val_buff), "errcode2=%d:", tmp_4_val);
                tmp_4_val = *(unsigned int *)(bf->recv_buff + 62);
                sprintf(bf->val_buff + strlen(bf->val_buff), "errcode3=%d:", tmp_4_val);
                tmp_4_val = *(unsigned int *)(bf->recv_buff + 66);
                sprintf(bf->val_buff + strlen(bf->val_buff), "errcode4=%d:", tmp_4_val);
                tmp_4_val = *(unsigned int *)(bf->recv_buff + 70);
                sprintf(bf->val_buff + strlen(bf->val_buff), "errcode5=%d:", tmp_4_val);
                tmp_4_val = *(unsigned int *)(bf->recv_buff + 74);
                sprintf(bf->val_buff + strlen(bf->val_buff), "yaoxin=%d", tmp_4_val);
                bf->val_buff[strlen(bf->val_buff)] = '\n';
                sprintf(bf->send_buff, "%s/%s/%08d%c", WORK_DIR, EXCEPTION_DIR, charger->CID, '\0');
                if ( (file = fopen(bf->send_buff, "ab+")) == NULL)
                    goto replay_exception;
                fwrite(bf->val_buff, 1, strlen(bf->val_buff), file);
                fclose(file);
replay_exception:
                if ( gernal_command(fd, CHARGER_CMD_HB_R, charger, bf) < 0)
                {
                    bf->ErrorCode = ESERVER_API_ERR;
                    free(tim);
                    return -1;
                }
                    free(tim);
                bf->ErrorCode = ESERVER_SEND_SUCCESS;
            return 0;;
		break;
        
        default:
            return -1;
        break;
	} // end  switch
    return -1;
}


void 
protocal_init_head(unsigned char CMD,  char *p_str, unsigned int cid)
{
	unsigned char len =0;
	strncpy(p_str, "EV<", 3);
#if 0
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
#endif 
	p_str[3] = 0;  //长度设置为0
	p_str[4] = CMD;
	p_str[5] = (unsigned char)(cid);
	p_str[6] = (unsigned char)(cid>>8);
	p_str[7] = (unsigned char)(cid>>16);
	p_str[8] = (unsigned char)(cid>>24);
	return ;	
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
	reuseaddr = 1;
    Setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,&reuseaddr,sizeof(reuseaddr));  // keepalive
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
        debug_msg("task-->cmd = %d", task->cmd);
        debug_msg("task-->cid = %d", task->cid);
        debug_msg("task-->way = %d", task->way);
        if (task->way == WEB_WAY)
        {
           for (i = 0; i < charger_manager.present_charger_cnt; i++)
           {
               if (task->cid == ChargerInfo[i].CID)
               {
	                 ev_uci_save_action(UCI_SAVE_OPT, true, "1", "chargerinfo.%s.STATUS", ChargerInfo[i].tab_name); 
                    // 来自页面的操作
                    switch (task->cmd)
                    {
                        case    WAIT_CMD_CHAOBIAO:
                                sprintf(val_buff, "%ld%c", time(0), '\0');
	                            ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CB_END_TIME", ChargerInfo[i].tab_name); 
	                            sprintf(val_buff, "%d%c", task->u.chaobiao.chargercode, '\0');
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
    time_t  start_time, end_time, init_time = 1262275200;
    char    CMD_TMP;
    char    index;
    struct  wait_task  cmd;
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
                wait_task_add(WEB_WAY, &cmd);
        break;
        case    WAIT_CMD_CONFIG:
                sprintf(val_buff, "%08d%c", CID, '\0');
                strcpy(cmd.u.config.name, val_buff);
                debug_msg("pthread of receive command is CONFIG");
                printf("val_buff:%s\n", cmd.u.config.name);
                wait_task_add(WEB_WAY, &cmd);
        break;
        case    WAIT_CMD_ONE_UPDATE:
                if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.TABS.chargerversion") < 0) 
                {
                    cmd.u.update.version[0] = 1;
                    cmd.u.update.version[1] = 6;
                }else
                {
                    cmd.u.update.version[0] = atoi(val_buff + 1);
                    cmd.u.update.version[1] = atoi(strchr(val_buff, '.')+1);
                    printf("version:%d %d\n", cmd.u.update.version[0], cmd.u.update.version[1]);
                }
                sprintf(val_buff, "%s%c", "CBMB.bin", '\0');
                strcpy(cmd.u.update.name, val_buff);
                wait_task_add(WEB_WAY, &cmd);
        break;
        case    WAIT_CMD_ALL_UPDATE:
               // if (ev_uci_data_get_val(val_buff, sizeof(val_buff), "chargerinfo.TABS.chargerversion") < 0) 
                //{
                    cmd.u.update.version[0] = 1;
                    cmd.u.update.version[1] = 6;
               // }else
               // {
                    printf("read file = %s\n", val_buff);
                    //strtok(bf->val_buff, ".");
                    //cmd.version[1] = atoi(strtok(NULL, "."));
                    for (i = 0; i < charger_manager.present_charger_cnt; i++)
                    {
                        if (ChargerInfo[i].free_cnt_flag == 1)
                        {
                            // 主版本
                            name[0] = val_buff[6];
                            name[1] = '\0';
                            cmd.u.update.version[0] = atoi(name);//atoi(val_buff + 1);

                            // 次版本
                            name[0] = val_buff[8];
                            name[1] = '\0';
                            cmd.u.update.version[1] = atoi(name);
                            // CID
                            cmd.cid = ChargerInfo[i].CID;
                            // 文件名
                            sprintf(val_buff, "%s%c", val_buff, '\0');
                            strcpy(cmd.u.update.name, val_buff);
                            wait_task_add(WEB_WAY, &cmd);
                            printf("将要更新的CID = %d\n", ChargerInfo[i]);
                        }
                    }
              //  }
                debug_msg("pthread of service read all update file = %s\n", ent->d_name);
        break;
        case    WAIT_CMD_ALL_CHAOBIAO:
                cmd.u.chaobiao.start_time = end_time; //start_time;
                cmd.u.chaobiao.end_time = time(0);//end_time;
                wait_task_add(WEB_WAY, &cmd);
        break;
      }
      // 加入等待队列
        ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.STATUS", "CLIENT"); //
        sprintf(val_buff, "%d%c", CMD_TMP, '\0');
	    ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CMD", "CLIENT");
        sprintf(val_buff, "%08d%c", CID, '\0');
	    ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CID", "CLIENT");

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
	    if(ev_uci_data_get_val(send_buff, 10, "chargerinfo.TABS.chargernum") >= 0)
        {
            if (atoi(send_buff) > 0)
                charger_manager.total_num = atoi(send_buff);
            printf("max_num = %d\n", charger_manager.total_num);
        }
	    if(ev_uci_data_get_val(send_buff, 10, "chargerinfo.TABS.max_current") >= 0)
        {
            if (atoi(send_buff) > 0)
                charger_manager.limit_max_current = atoi(send_buff);
            printf("max_current = %d\n", charger_manager.limit_max_current);
        }


          for (i = 0; i < CNT; i++)
          {
                if (ChargerInfo[i].present_cmd == 0x54) // 有拍卡充电
                {
                    have_charger_flag = 1;
                    charger_index = i;
                }
                if (ChargerInfo[i].is_charging_flag == 1)
                {
                    balance[i].charging_flag = 1;
                    NUM++;
                    
                } else
                {
                    balance[i].charging_flag = 0;      // balance 对应电桩序列,复制到局部变量，防止被改变
                }
                if (ChargerInfo[i].free_cnt_flag == 1 && ChargerInfo[i].free_cnt > 10)
                {
                    ChargerInfo[i].free_cnt_flag = 0;
                    
                }
			    if(ChargerInfo[i].free_cnt_flag == 1)
			    {
				    ChargerInfo[i].free_cnt++;
				    off_net_cnt++;	
			    }
                if (ChargerInfo[i].free_cnt_flag == 0)
                {
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
        sleep(10);
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
          printf("have_charger_flag = %d\n", have_charger_flag );
          if (have_charger_flag == 1)
          {
              printf("SSSSSSSSSSSSSSSSSSSSSSSSSS\nSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS有电桩充电\n");
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
                    printf("正在等待.....\n");
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
                printf("$$$$$$$$$$$$======>surpls:%d\n",surpls);
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
