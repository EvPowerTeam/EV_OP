


#include "./include/net_info.h"
#include "./include/serv_config.h"
#include "./include/CRC.h"
#include "./include/AES.h"
#include "./include/uci.h"

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

Thread_Type	*tptr;  // 全局指针，线程池地址

int	clifd[MAXCLI], iget, iput;	// 客户端连接数组
pthread_mutex_t  clifd_mutex;		// 客户端互斥锁
pthread_cond_t	clifd_cond;		// 线程条件变量
static  int nthreads;
pthread_mutex_t		clifd_mutex = PTHREAD_MUTEX_INITIALIZER;	// 线程互斥锁初始化
pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;		// 线程条件变量初始化
pthread_rwlock_t	charger_rwlock;



// 充电桩信息，数组
CHARGER_INFO_TABLE	ChargerInfo[10] = {0};  	// 用于存入每台电桩信息	
CHARGER_INFO_TABLE	*ChargerTable;			//没用
char			Table_Name[100];		//用于读取数据库所有表名信息 
static 	char		ChargerArrCnt = 0;		//用于充电桩个数
static	unsigned char	Charging_Num;			//正在充电的电桩个数
static  char		Charger_Max_Current = 32;	//最大默认电流
static  int		Charger_Record_Num = 0;		//先从数据库读取充电记录数，在此基础上增加电桩的充电记录

SERV_INFO  serv_info = {0};

// 函数声明
void thread_make(int i);
void protocal_init_head(unsigned char CMD, unsigned char *p_str, unsigned int cid);
int  sock_serv_init(void);
void * thr_fn(void * arg);
int  charger_serv(int fd, unsigned char *Index, int arg);
void daemonize(const char *cmd);
void msleep(unsigned long mSec);
int already_running(void);
void print_PresentMode(unsigned char pmode);
void print_SubMode(unsigned short submode);
void print_SelectSocket(unsigned char socket);
void key_init(unsigned char *key);
void clean_0x34_uci_database(unsigned char);
void  charger_info_init(void);
#define  CHARG_FILE	"/etc/config/chargerinfo"
int main(int argc , char * argv[])
{
	//服务器初始化
	// 设置信号处理
	//5.响应

	int i, connfd, listenfd;
	int	   err;		
	struct sockaddr *cliaddr;
 //       init_file("/etc/config/charger");
	if(access(CHARG_FILE,  F_OK) != 0)
	{
		init_file(CHARG_FILE); //CHARG_FILE);	//创建/etc/config/chargerinfo文件
		ev_uci_add_named_sec("chargerinfo.%s=tab", "TABS");
		ev_uci_add_named_sec("chargerinfo.%s=rod", "Record");
	}
//	ev_uci_add_named_sec("chargerinfo.%s = 2", "TABS");
	bzero(Table_Name, sizeof(Table_Name));
#if 0
        int result = ev_uci_add_named_sec("charger.general=general");
        printf("result: %d", result);
        ev_uci_add_sec("table2","charger"); //    不需要存储表名的话用这个
        ev_uci_add_named_sec("charger.general=general");
        ev_uci_add_named_sec("charger.charger1=CID1");          //ev_uci_add_named_sec("charger.CID1=name");  这样设计也可以
        ev_uci_add_named_sec("charger.charger2=CID2");
        result = ev_uci_save_val_string("abcdefghijk", "charger.charger1.ip");
        printf("result: %d", result);
        result = ev_uci_save_val_string("abcdefg", "charger.charger2.ip");
        printf("result: %d", result);
#endif 
	charger_info_init(); //数据库中读入信息，写入内存数组中，如果没有，也直接运行。(CID, IP, KEY)
	socklen_t addrlen, clilen;
#if 0
	daemonize(argv[0]);
	if(already_running())
	{
		syslog(LOG_ERR, "%s[%ld] is already running", argv[0], getpid());
		exit(1);
	}
	while(1);
#endif
#if 1
	listenfd = sock_serv_init();
	addrlen = sizeof(struct sockaddr_in);

	if( (cliaddr = malloc(addrlen)) == NULL)
		exit(1);
	// 线程相关初始化
	nthreads = atoi(argv[argc-1]);

	// 保护充电信息的读写锁，初始化
	if(pthread_rwlock_init(&charger_rwlock, NULL) < 0)
		exit(1);
	// 申请线程空间，全局
	if( (tptr = calloc(nthreads, sizeof(Thread_Type)) )== NULL)	 
		 exit(1);
	iget = iput = 0;
	
	// 创建所有线程
	for(i=0; i<nthreads; i++)
		thread_make(i);
	sleep(1);
	printf("线程创建成功...\n");

	for( ; ; )
	{
		printf("主线程开始监听...\n");
		clilen = addrlen;
		if((connfd = accept(listenfd, cliaddr, &clilen)) < 0 )
			continue;
		if(pthread_mutex_lock(&clifd_mutex) < 0)
		{
			// 出错处理
		}		
		clifd[iput] = connfd;
		if(++iput == iget)
		{
			// 错误判断
			exit(1);
		}
		if( iput == MAXCLI)
			iput = 0;

		if(pthread_cond_signal(&clifd_cond) < 0) // 唤醒等待条件的线程
		{

		}
		if(pthread_mutex_unlock(&clifd_mutex))
		{

		}
			
	}
#endif	
	return 0;
}

// 创建线程池
void thread_make(int i)
{
	void *thread_main(void *);
	if(pthread_create(&tptr[i].thread_tid, NULL, &thread_main, (void *)i) <0)
	{
		printf("pthread create err...\n");
		exit(1);
	}
	return ;
}


// 线程处理函数
void * thread_main(void * arg)
{
	int connfd;
	unsigned char	recv_buff[128] = {0};
	unsigned char	ChargerIndex;

	printf("线程%d开始运行...\n", (int)arg);

	for(;;)
	{
		if(pthread_mutex_lock(&clifd_mutex) < 0)
		{
			continue;
			// 出错处理
		}
		while(iget == iput)
		{
			
			if(pthread_cond_wait(&clifd_cond, &clifd_mutex) < 0) // 线程休眠，条件关闭，释放互斥锁
			{
				
			}
		}
	
//		printf("客户端有连接, 线程唤醒%d开始运行...\n", (int)arg);
		connfd = clifd[iget];
		if( ++iget == MAXCLI)
			iget = 0;
		
		printf("iget = %d , iput = %d\n", iget, iput);
		if(pthread_mutex_unlock(&clifd_mutex) < 0)
		{

		}
		// 执行服务程序
		
		charger_serv(connfd, &ChargerIndex, (int )arg);
		
		// 关闭连接
		sock_close(connfd);
		printf("线程关闭连接...\n");

	}


}

// 充电协议处理程序
int  charger_serv(int fd, unsigned char *Index, int arg)
{
			
	unsigned char	recv_buff[128] = {0};
	unsigned char   send_buff[128] = {0};
	unsigned int	declen = 0, enclen=0;
	int len, i;
	time_t 		tm =0;
	unsigned short	CRC = 0x1d0f, SubMode = 0, tmp_2_val = 0;
	unsigned int	CID =0, tmp_4_val = 0;
	unsigned char   ChargerCnt, CID_flag =0;
	// 协议初始化
	printf("线程%d进行服务程序...\n", arg);
	if( (len = read(fd, recv_buff, sizeof(recv_buff))) <= 0 ) // 客户端关闭连接
	{
		return 0;
	}

//	CID = (unsigned int)exchange_order(recv_buff+5, 4);
	CID = *(unsigned int *)(recv_buff+5);
	printf("################%d------------------->CMD = %#x, CID = %d\n", len, recv_buff[4], CID);
	ChargerCnt = ChargerArrCnt;
		// 根据CID查找充电信息表，索引
	printf("##################--------------->CNT=%d\n", ChargerArrCnt);
	if(pthread_rwlock_wrlock(&charger_rwlock) < 0)
		return 0;
	for(i=0; i<ChargerCnt; i++)
	{
		printf("内存中CID ========>%d\n", ChargerInfo[i].CID);
		if( memcmp(&ChargerInfo[i].CID, &CID, 4) == 0) // 判断CID是否相同
		{
			printf("找到的CID========>%d\n", ChargerInfo[i].CID);
				*Index = i;
				ChargerInfo[i].flag_cnt = 1;
				CID_flag = 1;
				//break;	
				continue;
		}
		ChargerInfo[i].flag_cnt++;
		if( ChargerInfo[i].flag_cnt %100 == 0 && ChargerInfo[i].flag == 1) // 解决正在充电时，充电桩断开，load balance分配电流不正确的问题
		{
			ChargerInfo[i].flag = 0;
			Charging_Num--;
		}
	}
	
	if(pthread_rwlock_unlock(&charger_rwlock) < 0)
		return 0;
	// 数据处理, 希望接收数据达到一定的个数在处理，否则丢掉，减少解密处理
	// 1.数据解密,CRC判断，帧头判断略

	// 数据处理, 希望接收数据达到一定的个数在处理，否则丢掉，减少解密处理
	// 1.数据解密,CRC判断，帧头判断略
	if( recv_buff[4] !=  0x10 )// 找到匹配的
	{
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
		printf("CRC校验失败...\n");
		//出错处理
		return 0;
	}
	// 清零
	bzero(send_buff, sizeof(send_buff));
	unsigned char val_buff[200] = {0};

	switch(recv_buff[4]){
		case	0x10:	// 连接请求
			// 用读的方式，锁住读写锁，然后查看信息,未实现
			printf("连接请求...\n");
			unsigned char *key_addr;
			unsigned char tab_buff[20] = {0};
			char * p = Table_Name;
			if( pthread_rwlock_wrlock(&charger_rwlock) < 0){
				goto cmd_0x10;
			}
			
			if( (key_addr = (unsigned char *)malloc(16)) == NULL ) //生成随即KEYB值
				return 0;
			bzero(key_addr, 16);
			key_init(key_addr);	//获取随机KEYB值

			// 将接收到的CID
			// 查询充电信息更新
			
			if(CID_flag == 1) // 一个断了的重新连接，只需要更新IP
			{
					memcpy(ChargerInfo[(*Index)].IP, recv_buff+9, 4);
					memcpy(ChargerInfo[(*Index)].KEYB, key_addr, 16);
					//将改变的IP存入数据库
//					unsigned ip_buff[20] = {0};

					sprintf(val_buff, "%d.%d.%d.%d", recv_buff[9], recv_buff[10], recv_buff[11], recv_buff[12]);
					if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.IP", ChargerInfo[(*Index)].tab_name) <0)	// 存入IP
					//if(ev_uci_save_action(val_buff, "chargerinfo.%s.IP", ChargerInfo[(*Index)].tab_name) <0)	// 存入IP
						goto cmd_0x10;
					memcpy(val_buff, key_addr, 16);
					if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.KEYB", ChargerInfo[(*Index)].tab_name) < 0)
					//if(ev_uci_save_action(val_buff, "chargerinfo.%s.KEYB", ChargerInfo[(*Index)].tab_name) < 0)
						goto cmd_0x10;
					bzero(val_buff, sizeof(val_buff));
					strncpy(val_buff, recv_buff+18, 10);
					if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Model", ChargerInfo[(*Index)].tab_name) < 0) // series
						goto cmd_0x10;
					strncpy(val_buff, recv_buff+28, 10);
					if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Series", ChargerInfo[(*Index)].tab_name) < 0)	
						goto cmd_0x10;
					bzero(val_buff, strlen(val_buff));
					sprintf(val_buff, "%d.%02d", recv_buff[41], recv_buff[40]);	//chargerVersion
					if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerVersion", ChargerInfo[(*Index)].tab_name) < 0)	
						goto cmd_0x10;
					bzero(val_buff, sizeof(val_buff));
					strncpy(val_buff, recv_buff+18, 10);
					printf("====================连接请求: %s\n", val_buff);
					if(!strcmp(val_buff, "EVG-16N"))
					{
						ChargerInfo[(*Index)].model = EVG_16N;
					}else if(!strcmp(val_buff, "EVG-32N"))
					{
						ChargerInfo[(*Index)].model = EVG_32N;	
					}
					printf("========================>mode = %d\n", ChargerInfo[(*Index)].model);
					
			}
			else
			{
					//全新的连接，初始化key值
					// 更新数据库
#if 1
					if(strlen(Table_Name) == 0)
					{
						sprintf(tab_buff, "charger%d", ChargerArrCnt+1);
					}else
					{
						sprintf(tab_buff, ",charger%d", ChargerArrCnt+1);
					}
					strcpy(Table_Name+strlen(Table_Name), tab_buff); //追加表名
					if(ev_uci_save_action(UCI_SAVE_OPT, true, Table_Name, "%s", TAB_POS) < 0) // 保存数据到数据库
						//return 0;
						goto cmd_0x10;
					// 保存CID，IP， KEYD到数据库
					// 创建表，选项,未实现
					bzero(tab_buff,  sizeof(tab_buff));
					sprintf(tab_buff, "charger%d", ChargerArrCnt+1);
					sprintf(val_buff, "%d", CID);
					printf("tab_buff ===%s\n", tab_buff);
					//ev_uci_add_named_sec("chargerinfo.%s=%d", tab_buff, ChargerArrCnt+1);
					ev_uci_add_named_sec("chargerinfo.%s=1", tab_buff);
					usleep(10);	
					if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.CID", tab_buff )  < 0)
					//if(ev_uci_save_action_string(val_buff, "chargerinfo.%s.CID", tab_buff )  < 0)
						//return 0;
						goto cmd_0x10;
					sprintf(val_buff, "%d.%d.%d.%d", recv_buff[9], recv_buff[10], recv_buff[11], recv_buff[12]);	// IP
					if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.IP", tab_buff )  < 0)
					//if(ev_uci_save_action(val_buff, "chargerinfo.%s.IP", tab_buff )  < 0)
						//return 0;
						goto cmd_0x10;
					memcpy(val_buff, key_addr, 16);
					if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.KEYB", tab_buff)  < 0)
					//if(ev_uci_save_action(val_buff, "chargerinfo.%s.KEYB", tab_buff)  < 0)
						goto cmd_0x10;
						//return 0;
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
				//	usleep(100);
#endif
					// 写入数据库
					ChargerInfo[ChargerArrCnt].CID = CID;
//					memcpy(&ChargerInfo[ChargerArrCnt].CID, &CID, 4);
//					ChargerInfo[ChargerArrCnt].CID[0] = recv_buff[5];					
//					ChargerInfo[ChargerArrCnt].CID[1] = recv_buff[6];					
//					ChargerInfo[ChargerArrCnt].CID[2] = recv_buff[7];					
//					ChargerInfo[ChargerArrCnt].CID[3] = recv_buff[8];					
					ChargerInfo[ChargerArrCnt].IP[0] = recv_buff[9];
					ChargerInfo[ChargerArrCnt].IP[1] = recv_buff[10];
					ChargerInfo[ChargerArrCnt].IP[2] = recv_buff[11];
					ChargerInfo[ChargerArrCnt].IP[3] = recv_buff[12];
					memcpy(ChargerInfo[ChargerArrCnt].KEYB, key_addr, 16);
					strcpy(ChargerInfo[ChargerArrCnt].tab_name, tab_buff);
					strncpy(val_buff, recv_buff+18, 10);
					printf("====================连接请求: %s\n", val_buff);
					if(!strcmp(val_buff, "EVG-16N"))
					{
						ChargerInfo[ChargerArrCnt].model = EVG_16N;
					}else if(!strcmp(val_buff, "EVG-32N"))
					{
						ChargerInfo[ChargerArrCnt].model = EVG_32N;	
					}
					printf("========================>mode = %d\n", ChargerInfo[ChargerArrCnt].model);
//					ChargerInfo[ChargerArrCnt].flag = 1;
					ChargerArrCnt++;
		  	 }
			if(pthread_rwlock_unlock(&charger_rwlock) < 0){
				//return 0;
				goto cmd_0x10;
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
			printf("发送CRC[0] =%#x CRC[1]=%#x\n", (CRC&0xff), (CRC>>8));
			send_buff[CMD_0X11_LEN+2] = (unsigned char)(CRC);
			send_buff[CMD_0X11_LEN+3] = (unsigned char)(CRC>>8);
			// 加密，发送
			Padding(send_buff+9, CMD_0X11_LEN-5, recv_buff, &enclen);
#ifndef NDEBUG
			printf("发送加密之前数据:");
			for(i = 0; i< enclen+9; i++)
			{
				printf("%#x ", send_buff[i]);
			}
			printf("\n");
#endif		
			My_AES_CBC_Encrypt(KEYA, recv_buff, enclen, send_buff+9);
#ifndef NDEBUG
			printf("发送加密数据...\n");
			for(i = 0; i< enclen+9; i++)
			{
				printf("%#x ", send_buff[i]);
			}

			printf("\n");
#endif
			msleep(100);
			if(write(fd, send_buff, enclen+9) != enclen+9)
			{
				goto cmd_0x10;
				//出错
			}
#ifndef NDEBUG
			printf("回应连接请求成功...长度=%d\n", enclen+9);
#endif
			free(key_addr);
			return 1;
cmd_0x10:
			free(key_addr);
			return 0;
			// 发送回应
		break;
		case 	0x34:	// 心跳
			printf("心跳请求...\n");
			//if(ChargerInfo[(*Index)].flag == 0)
			//	return 0;
			SubMode = *(unsigned short *)(recv_buff+10);
			print_PresentMode(recv_buff[9]);
			print_SubMode(SubMode);
			if(pthread_rwlock_wrlock(&charger_rwlock) < 0)
			{
				return 0;
			}
			if(ChargerInfo[(*Index)].model ==0)
			{
				send_buff[13] = 8;//16 ;//(ChargerInfo[(*Index)].model); //Charger_Max_Current/1;//支持最大的电流
				ChargerInfo[(*Index)].model = 8;//16;
			}else
			{
					send_buff[13] = 8;//ChargerInfo[(*Index)].model;
			}
			if(recv_buff[9] ==  CHARGER_CHARGING_COMPLETE_LOCK && ChargerInfo[(*Index)].flag == 0)  //解决充电完成，还未解锁导致浮点计算出错的BUG
			{
				Charging_Num++;
				ChargerInfo[(*Index)].flag = 1;
			}
			sprintf(val_buff, "%d", recv_buff[9]);	//presentMode
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentMode", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			sprintf(val_buff, "%d", SubMode); // SUB_MODE
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SubMode", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			 clean_0x34_uci_database(*Index); //将数据库电桩信息的数据字段置0
			if(pthread_rwlock_unlock(&charger_rwlock) < 0)
			{
				return 0;
			}
			// 心跳处理略
			//  回复心跳
			protocal_init_head(0x35, send_buff, CID);
			tm = time(0);
			send_buff[9] = (unsigned char)tm;	//时间戳
			send_buff[10] = (unsigned char)(tm>>8);
			send_buff[11] = (unsigned char)(tm>>16);
			send_buff[12] = (unsigned char)(tm>>24);
			printf("===========================>0x34 mode = %d\n", ChargerInfo[(*Index)].model);
			CRC = getCRC(send_buff, CMD_0X35_LEN+2);	
			send_buff[14] = (unsigned char)CRC;		// CRC
			send_buff[15] = (unsigned char)(CRC >> 8);
			//加密
			bzero(recv_buff, sizeof(recv_buff));
			Padding(send_buff+9, CMD_0X35_LEN-5, recv_buff, &enclen);
			My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
			msleep(100);
			
			if(write(fd, send_buff, enclen+9) != enclen+9){
				return 0;
			}
				
			printf("回应心跳成功%d...\n", Charging_Num);
			
		break;
		case	0x54:	//充电请求
			printf("充电请求...\n");
			unsigned int AccPower = *(unsigned int *)(recv_buff+22);
			unsigned short AccPowerCnt = *(unsigned short *)(recv_buff+26);
			unsigned char 	SelectCurrent = recv_buff[29];
			unsigned char  PresentOutputCurrent = recv_buff[30];
			unsigned short  ChargTime = *(unsigned short *)(recv_buff+31);
			char cnt_0x54 = 0;
			// 统计充电请求个数
			//记录客户信息
			char * p_0x54 = ChargerInfo[(*Index)].ev_linkidtmp;
			ChargerInfo[(*Index)].start_time = time(0);	//开始充电时间
			for(i = 0; i< 16; i++)
			{
				sprintf(p_0x54, "%02x", recv_buff[35+i]);
				p_0x54 +=2;
			}
			memcpy(ChargerInfo[(*Index)].ev_linkid, recv_buff+35, 16);		//evlink id
			ChargerInfo[(*Index)].charging_code = *(unsigned short *)(recv_buff+33);			//赋值charging_code
			if( pthread_rwlock_wrlock(&charger_rwlock) < 0){
				return 0;
			}
			sprintf(val_buff, "%d", ChargerInfo[(*Index)].start_time);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargStartTime", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[17]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentMode", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			tmp_2_val = *(unsigned short *)(recv_buff + 18);
			sprintf(val_buff, "%d", tmp_2_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SubMode", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			bzero(val_buff, strlen(val_buff));
			tmp_4_val = *(unsigned int *)(recv_buff + 22);
			sprintf(val_buff, "%d", tmp_4_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.AccPowerStart", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;	
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[28]);	//select socket
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectSocket", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[29]);	//select current
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectCurrent", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[30]);	//presentoutputcurrent
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentOutputCurrent", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			tmp_2_val = *(unsigned short *)(recv_buff+31);
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", tmp_2_val);	//duration
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargingDuration", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			tmp_2_val = *(unsigned short *)(recv_buff+33);
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", tmp_2_val);	//duration
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargingCode", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			if(ev_uci_save_action(UCI_SAVE_OPT, true, ChargerInfo[(*Index)].ev_linkidtmp, "chargerinfo.%s.EVLinkID", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[55]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerWay", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;	
			Charging_Num++;	// 充电桩正在充电的个数

			if(Charging_Num == 1)
			{
				send_buff[35] = 0;//ChargerInfo[(*Index)].model;
				
			}
			else
			{
				send_buff[35] = 0;	
			}
			ChargerInfo[(*Index)].change_current = send_buff[35];	
			
			if(pthread_rwlock_unlock(&charger_rwlock) < 0){
				return 0;
			}
			ChargerInfo[(*Index)].flag = 1;
			SubMode = *(unsigned short *)(recv_buff+18);
			print_SelectSocket(recv_buff[28]);
			print_PresentMode(recv_buff[17]);
			print_SubMode(SubMode);
			printf("#####----->AccPower:%d\n", AccPower);
			printf("#####----->AccCnt:%d\n", AccPowerCnt);
			printf("#####----->选择电流:%d\n", SelectCurrent);
			printf("#####----->当前输出电流:%d\n", PresentOutputCurrent);
			printf("#####----->卡号:");
			for(i=0; i<16;i++)
			{
				printf("%#x ", recv_buff[35+i]);
			}				
			printf("\n");
			tm = *(unsigned int *)(recv_buff+13); 
			struct tm * tim = localtime(&tm);
			printf("开始充电时间为%d---------->%d-%d-%d %d:%d:%d\n", tm, tim->tm_year+1900, tim->tm_mon+1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);

			//回应充电请求
			//protocal_init_head(0x55, send_buff, ChargerInfo[(*Index)].CID);
			protocal_init_head(0x55, send_buff, *(unsigned int *)(recv_buff+5));
			tm = time(0);
			send_buff[9] = (unsigned char)tm;	//时间戳
			send_buff[10] = (unsigned char)(tm>>8);
			send_buff[11] = (unsigned char)(tm>>16);
			send_buff[12] = (unsigned char)(tm>>24);
			send_buff[13] = CHARGER_CHARGING;	// targetMode 状态改变
			send_buff[14] = 0x01;			// systemMessage
			send_buff[15] = (unsigned char)2880;
			send_buff[16] = (unsigned char)(2880 >> 8); //ChargingDuration
			send_buff[17] = (unsigned char)(10);	// chargingCode  充电记录
			send_buff[18] = (unsigned char)(10 >> 0);
			memcpy(send_buff+19, recv_buff+35, 16);	// EVlik 卡
#if 0
			if(Charger_Max_Current/Charging_Num >= ChargerInfo[(*Index)].model)
			{
				send_buff[35] = ChargerInfo[(*Index)].model;
				ChargerInfo[(*Index)].change_current = ChargerInfo[(*Index)].model;
				printf("========================================>连接请求 evg = %d\n", send_buff[35]);
			}
			else
			{
				send_buff[35] = Charger_Max_Current/Charging_Num; // MAX current
				ChargerInfo[(*Index)].change_current = Charger_Max_Current/Charging_Num;
				printf("========================================>连接请求 evg = %d\n", send_buff[35]);
			}
			ChargerInfo[(*Index)].change_current_cnt = 0;
#endif		
#if 0
 			printf("0x54======================================>recv_buff[29] = %d\n", recv_buff[29]);	
			if( (recv_buff[29] !=  0 )&& (recv_buff[29]%16 == 0))
			{
				ChargerInfo[(*Index)].model = recv_buff[29];
			} 
			if(Charger_Max_Current/Charging_Num > ChargerInfo[(*Index)].model)	//select current
			{
				send_buff[35] = ChargerInfo[(*Index)].model;	
			}
			else
			{
				send_buff[35] = Charger_Max_Current/Charging_Num;
			}
#endif
			send_buff[35] = 8;
			printf("0x54===================================>%d\n", send_buff[35]);
			CRC = getCRC(send_buff, CMD_0X55_LEN+2);
			send_buff[36] = (unsigned char)CRC;
			send_buff[37] = (unsigned char)(CRC >> 8);
			bzero(recv_buff, sizeof(recv_buff));	
			Padding(send_buff+9, CMD_0X55_LEN-5, recv_buff, &enclen);
#if 0
			for(i = 0; i< enclen; i++)
			{
				printf("%#x ", recv_buff[i]);
			}
			printf("\n");
#endif
			My_AES_CBC_Encrypt((ChargerInfo[(*Index)].KEYB), recv_buff, enclen, send_buff+9);
			msleep(100);
			if(write(fd, send_buff, enclen+9) != enclen+9)
			{
				return 0;
			}
			printf("回应充电请求成功%d...长度=%d\n", Charging_Num, enclen+9);
		break;
		case	0x56:	//充电中状态更新
			printf("充电中状态更新...\n");
			SubMode = *(unsigned short *)(recv_buff+18);
			print_PresentMode(recv_buff[17]);
			print_SubMode(SubMode);
			printf("当前已充电时间:%0.2f小时\n", *(unsigned short *)(recv_buff+23) / 60.0);
			printf("当前已充电电量:%0.2fKWh\n", *(unsigned short *)(recv_buff+25)/1000.0);
			tm = *(unsigned int *)(recv_buff+13); 
			tim = localtime(&tm);
			printf("充电时间为%d---------->%d-%d-%d %d:%d:%d\n", tm, tim->tm_year+1900, tim->tm_mon+1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);

			if(pthread_rwlock_wrlock(&charger_rwlock) < 0)
				return 0;
			if(recv_buff[17] == CHARGER_CHARGING && ChargerInfo[(*Index)].flag == 0)  //解决服务器短暂断开重新连接
			{
				Charging_Num++;	// 充电桩正在充电的个数
				ChargerInfo[(*Index)].flag = 1;
			}


			tmp_2_val = *(unsigned short *)(recv_buff+32);
			usleep(10);
		printf("=================>正在充电的电桩数:%d\n", Charging_Num);
			if(tmp_2_val != ChargerInfo[(*Index)].charging_code)  //判断断了重新充电是否是新纪录，是则把原来的写入数据库
			{
					// 新充电用户
				Charger_Record_Num++;
					//将就的信息写入数据库
				bzero(val_buff, strlen(val_buff));
				sprintf(val_buff, "%d", Charger_Record_Num);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.record.%s", "nums")  < 0)
					return  0;
				sprintf(val_buff, "%ld,%ld,%ld,%d,%d,%s", CID, ChargerInfo[(*Index)].start_time, 0/*ChargerInfo[(*Index)].end_time*/, ChargerInfo[(*Index)].power,recv_buff[31], ChargerInfo[(*Index)].ev_linkidtmp);
				if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.record.num%d", Charger_Record_Num ) < 0)
				return  0;
				ChargerInfo[(*Index)].charging_code =  tmp_2_val;	
			}
			sprintf(val_buff, "%d", recv_buff[17]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentMode", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[22]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentOutputCurrent", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short *)(recv_buff+23);
			sprintf(val_buff, "%d", tmp_2_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Duration", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short *)(recv_buff+25);
			sprintf(val_buff, "%d", tmp_2_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Power", ChargerInfo[(*Index)].tab_name) <0 )
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[31]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerWay", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;	
			
			if(pthread_rwlock_unlock(&charger_rwlock) < 0)
				return 0;
			protocal_init_head(0x35, send_buff, CID);
			tm = time(0);
			send_buff[9] = (unsigned char)tm;	//时间戳
			send_buff[10] = (unsigned char)(tm>>8);
			send_buff[11] = (unsigned char)(tm>>16);
			send_buff[12] = (unsigned char)(tm>>24);
#if 0			
			int cnt;
			for(i=0; i<ChargerArrCnt; i++)
			{
				if(i != *Index && ChargerInfo[i].flag == 1)
				{
					send_buff[13] = 0;
					ChargerInfo[i].change_current = send_buff[13];
				}
			}
			if((Charger_Max_Current/Charging_Num) > ChargerInfo[(*Index)].model)
			{
				if(recv_buff[34] == ChargerInfo[(*Index)].change_current)
			}
#endif

			send_buff[13] = 8;
#if 0
			if((Charger_Max_Current/Charging_Num) > ChargerInfo[(*Index)].model)
			{
				send_buff[13] = ChargerInfo[(*Index)].model;
				printf("model current ==---------------------->%d\n", send_buff[13]);
			}
			else
			{
				send_buff[13] = Charger_Max_Current/Charging_Num;
				printf("load balance current ==---------------------->%d\n", send_buff[13]);
			}
			
#endif
			
#if 0
			if(ChargerInfo[(*Index)].change_current_cnt == 15)
			{	
				if((Charger_Max_Current/Charging_Num) >= ChargerInfo[(*Index)].model)
				{
					send_buff[13] = (unsigned char)ChargerInfo[(*Index)].model;
					ChargerInfo[(*Index)].change_current = send_buff[13];
					ChargerInfo[(*Index)].change_current_cnt = 0;
				}
				else
				{
					send_buff[13] = (unsigned char)Charger_Max_Current/Charging_Num; // MAX current	
					ChargerInfo[(*Index)].change_current = send_buff[13];
					ChargerInfo[(*Index)].change_current_cnt = 0;
				}
				printf("==============================================================>改变电流:%d\n", send_buff[13]);
					
			}
			else
			{
				send_buff[13] = ChargerInfo[(*Index)].change_current;
				ChargerInfo[(*Index)].change_current_cnt++;
					
				printf("==============================================================>未改变电流:%d\n", send_buff[13]);
			}
#endif

			CRC = getCRC(send_buff, CMD_0X35_LEN+2);	
			send_buff[14] = (unsigned char)CRC;		// CRC
			send_buff[15] = (unsigned char)(CRC >> 8);
			//加密
			bzero(recv_buff, sizeof(recv_buff));
			Padding(send_buff+9, CMD_0X35_LEN-5, recv_buff, &enclen);
			My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
			msleep(100);
			if(write(fd, send_buff, enclen+9) != enclen+9){
				return 0;
			}
			printf("回应充电中状态成功%d...\n", Charging_Num);

		break;
		case	0x58:	//停止充电请求
			printf("停止充电请求...\n");
			ChargerInfo[(*Index)].end_time = time(0);				//获取充电结束时间
			ChargerInfo[(*Index)].power = *(unsigned short *)(recv_buff+37);	//获取结束充电的电量
			if(pthread_rwlock_wrlock(&charger_rwlock) < 0)
			{
				return 0;
			}
			Charger_Record_Num++;	//充电记录增加
			//写入数据库，更新相应表信息
			sprintf(val_buff, "%d", *(unsigned short *)(recv_buff+33));
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.recordcnt", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", Charger_Record_Num);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.Record.%s", "nums")  < 0)
				return  0;
			sprintf(val_buff, "%ld,%ld,%ld,%d,%d,%s", CID, ChargerInfo[(*Index)].start_time, ChargerInfo[(*Index)].end_time, ChargerInfo[(*Index)].power,recv_buff[59], ChargerInfo[(*Index)].ev_linkidtmp);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.Record.num%d", Charger_Record_Num ) < 0)
				return  0;
			bzero(val_buff, strlen(val_buff));
			//当前模式
			sprintf(val_buff, "%d", ChargerInfo[(*Index)].end_time);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargEndTime", ChargerInfo[(*Index)].tab_name) < 0)
			return 0;
			sprintf(val_buff, "%d", recv_buff[17]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentMode", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short*)(recv_buff+18);
			sprintf(val_buff, "%d", tmp_2_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SubMode", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
		
			bzero(val_buff, strlen(val_buff));
			tmp_4_val = *(unsigned int *)(recv_buff +22);
			sprintf(val_buff, "%d", tmp_4_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.AccPowerEnd", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[28]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectSocket", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[29]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.SelectCurrent", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;	
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[30]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.PresentOutputCurrent", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;	
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short*)(recv_buff+31);
			sprintf(val_buff, "%d", tmp_2_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargingDuration", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short*)(recv_buff+33);
			sprintf(val_buff, "%d", tmp_2_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargingCode", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short*)(recv_buff+35);
			sprintf(val_buff, "%d", tmp_2_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Duration", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;
			bzero(val_buff, strlen(val_buff));
			tmp_2_val = *(unsigned short*)(recv_buff+37);
			sprintf(val_buff, "%d", tmp_2_val);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Power", ChargerInfo[(*Index)].tab_name) < 0)
				return 0;				
			bzero(val_buff, strlen(val_buff));
			sprintf(val_buff, "%d", recv_buff[59]);
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerWay", ChargerInfo[(*Index)].tab_name));	
	
			Charging_Num--;
			if(pthread_rwlock_unlock(&charger_rwlock)< 0)
			{
				return 0;
			}
			ChargerInfo[(*Index)].flag = 0;
			SubMode = *(unsigned short *)(recv_buff+18);
			print_SelectSocket(recv_buff[28]);
			print_PresentMode(recv_buff[17]);
			printf("停止充电时间:%0.2f小时\n", *(unsigned short *)(recv_buff+35) / 60.0);
			printf("停止充电电量:%0.2fKWh\n", *(unsigned short *)(recv_buff+37)/1000.0);
			tm = *(unsigned int *)(recv_buff+13); 
			tim = localtime(&tm);
			printf("停止充电时间为%d---------->%d-%d-%d %d:%d:%d\n", tm, tim->tm_year+1900, tim->tm_mon+1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);
			printf("#####----->卡号:");
			for(i=0; i<16;i++)
			{
				printf("%#x ", recv_buff[39+i]);
			}				
			printf("\n");

			protocal_init_head(0x57, send_buff, CID);
			tm = time(0);
			send_buff[9] = (unsigned char)tm;	//时间戳
			send_buff[10] = (unsigned char)(tm>>8);
			send_buff[11] = (unsigned char)(tm>>16);
			send_buff[12] = (unsigned char)(tm>>24);
			send_buff[13] = (unsigned char)CHARGER_READY;
			send_buff[14] = 0x02;
//			memcpy(send_buff+15, recv_buff+39, 16);	// EVlik 卡
			memcpy(send_buff+15, ChargerInfo[(*Index)].ev_linkid, 16);	// EVlik 卡
			CRC = getCRC(send_buff, CMD_0X57_LEN+2);	
			send_buff[31] = (unsigned char)CRC;		// CRC
			send_buff[32] = (unsigned char)(CRC >> 8);

			bzero(recv_buff, sizeof(recv_buff));
			Padding(send_buff+9, CMD_0X57_LEN-5, recv_buff, &enclen);
			My_AES_CBC_Encrypt(ChargerInfo[(*Index)].KEYB, recv_buff, enclen, send_buff+9);
			msleep(100);
			if(write(fd, send_buff, enclen+9) != enclen+9){
				return 0;
			}
			
			printf("回应停止充电请求成功...%d\n", enclen+9);
		break;
		case	0xa3:	//控制电桩回应
		break;
		case	0xa5:	//抄表回应
		break;
		case	0x91:	//更新指令回应
		break;
		case	0x95:	//更新软件回应
		break;


	} // end  switch

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
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("sock");
		exit(1);
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERV_PORT);
	addr.sin_addr.s_addr = inet_addr(local_ipbuf);
	
	// 地址重用
	int reuseaddr = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr));

	// 地址绑定
	if(bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("bind");
		exit(1);
	}
	
	// 监听
	if(listen(sockfd, 100) < 0)
	{
		perror("listen");
		exit(1);
	}
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
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Power", ChargerInfo[(Index)].tab_name) < 0)
				return ;
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargingDuration", ChargerInfo[(Index)].tab_name) < 0)
				return ;
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.Duration", ChargerInfo[(Index)].tab_name) < 0)
				return ;
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargStartTime", ChargerInfo[(Index)].tab_name) < 0)
				return ;
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargEndTime", ChargerInfo[(Index)].tab_name) < 0)
				return ;
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargerWay", ChargerInfo[(Index)].tab_name) < 0)
				return ;
			if(ev_uci_save_action(UCI_SAVE_OPT, true, val_buff, "chargerinfo.%s.ChargingCode", ChargerInfo[(Index)].tab_name) < 0)
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
		usleep(5);
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


void msleep(unsigned long mSec){
    struct timeval tv;
    tv.tv_sec=mSec/1000;
    tv.tv_usec=(mSec%1000)*1000;
    int err;
    do{
       err=select(0,NULL,NULL,NULL,&tv);
    }while(err<0 && errno==EINTR);
}


//取出数据库配置信息，然后存入内存中
void  charger_info_init(void)
{
	//查询数据库，获取表名数据
	unsigned  char *tab_name = NULL;
	unsigned char *tab_tmp = NULL;
	unsigned char *str;
	unsigned char name[10][10] = {0}, info[20], i = 0, j=0, len, charg_cnt = 0;
	printf("uci  init ... \n");
	if( (tab_tmp = tab_name = find_uci_tables(TAB_POS)) == NULL)
		return ;
	while(*tab_name)
	{
		if(*tab_name == ',')
		{
			i++;
			j = 0;
			tab_name++;
			continue;
		}
		name[i][j++] = *tab_name++;		
	}
	printf("char_info_init = %s\n", name[0]);
	for(j =0; j<i+1; j++)
	{
		if(ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.',  "CID") < 0){ //获取CID
			continue;
		}
		ChargerInfo[j].CID = atoi(info);
#ifndef NDEBUG
		printf("数据库取出的CID为------------------>%d\n", ChargerInfo[charg_cnt].CID);
#endif	

		bzero(info, strlen(info));
		if(ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.', "IP") < 0) // IP
			continue;
		//ChargerInfo[j].IP = inet_addr(info);
		unsigned char* p = info;
		char cnt;
		for(cnt = 0; cnt < 4; cnt++)
		{
			if(cnt == 0){
				str = strtok(p, ".");
				if(str) 	ChargerInfo[charg_cnt].IP[cnt] = atoi(str);
				continue;
			}
			str = strtok(NULL, ".");
			if(str)
				ChargerInfo[charg_cnt].IP[cnt] = atoi(str);
		}	
#ifndef NDEBUG
		printf("数据库取出的IP为------------------>%d.%d.%d.%d\n", ChargerInfo[charg_cnt].IP[0], ChargerInfo[charg_cnt].IP[1], ChargerInfo[charg_cnt].IP[2], ChargerInfo[charg_cnt].IP[3]);
#endif	
		bzero(info, strlen(info));
		// 赋值key值
		if(ev_uci_data_get_val(info, 20, "chargerinfo.%s%c%s", name[j], '.', "KEYB") < 0)
			continue;
		int c;
		for(c=0; c<16; c++)
			printf("%d ", info[c]);
		printf("\n");
		strncpy(ChargerInfo[charg_cnt].KEYB, info, 16);
		strcpy(ChargerInfo[charg_cnt].tab_name, name[j]);
		charg_cnt++;
		ChargerArrCnt++;	// 全局数组，用于在内存中记录充电桩的个数,所有线程共享	
#ifndef NDEBUG
		printf("当前从数据库取出充电桩个数为--------------->%ld\n", ChargerArrCnt);
#endif
	}
	memcpy(Table_Name, tab_tmp, strlen(tab_tmp));	//赋值表名到全局数组
	free(tab_tmp);
	
	// 存取充电记录变量
	bzero(info, sizeof(info));
	if(ev_uci_data_get_val(info, 20, "chargerinfo.Record.nums") < 0)
	{
		return;	
	}
		
	if(strlen(info) == 0)
	{
		Charger_Record_Num = 0; //数据库没有充电记录，置0
		return;
	}
	Charger_Record_Num = atoi(info);
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


