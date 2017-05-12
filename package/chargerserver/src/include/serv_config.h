

#ifndef __SERV_CONFIG__H
#define __SERV_CONFIG__H


#include <pthread.h>
#include <sys/types.h>
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
//#include <mqueue.h>

//#define  	NDEBUG
#ifdef __GNUC__
#define UNUSED    __attribute__((unused))
#endif

#ifndef __GNUC__
#define __attribute__(x)   /*Nothing*/
#define UNUSED             /*Nothing*/
#endif

#ifndef   NDEBUG
#include <assert.h>
#define DEBUG_ERR(format,...) 	printf("FILE: "__FILE__", LINE: %d: "format"\n", __LINE__, ##__VA_ARGS__)
#define DEBUG(...)		printf(__VA_ARGS__)
#endif

typedef  unsigned char  u8;
typedef  unsigned short u16;
typedef  unsigned int   u32;
typedef  unsigned char  ev_uchar;
typedef  unsigned short ev_ushort;
typedef  unsigned int   ev_uint;
typedef  unsigned long  ev_ulong;
typedef  char           ev_char;
typedef  short          ev_short;
typedef  int            ev_int;
typedef  long           ev_long;



/******************************************************
 *
 *      充电协议相关
 *
 *****************************************************/

// 后台发送命令错误码
#define         MSG_CMD_FROM_CHARGER           99
#define         EMSG_HAVE_NO_CHARGER_ERROR      100
#define         EMSG_CONFIG_ERROR               101
#define         EMSG_CHAOBIAO_ERROR             102
#define         EMSG_UPDATE_ERROR               103
#define         EMSG_YUYUE_ERROR                104
#define         EMSG_START_CHARGE_ERROR         105
#define         EMSG_STOP_CHARGE_ERROR          106
#define         EMSG_CONTROL_ERROR              107
#define         EMSG_OFF_NET                    108
#define         EMSG_START_CHARGE_CLEAN         109
#define         EMSG_STOP_CHARGE_CLEAN          110

// errcode 
//电桩的状态，定义在CHARGER_INFO_TABLE结构体中的present_status成员中赋值
#define         CHARGER_STATE_RESTART           499
#define         CHARGER_STATE_CONFIG            500
#define         CHARGER_STATE_UPDATE            501
#define         CHARGER_STATE_CHAOBIAO          502
#define         CHARGER_STATE_OFF_NET           503
// 后台发送命令到电桩的状态,定义在CHARGER_INFO_TABLE结构体中的server_send_status成员中赋值
#define         MSG_STATE_NULL                  504
#define         MSG_STATE_CONFIG                505
#define         MSG_STATE_UPDATE                506
#define         MSG_STATE_CHAOBIAO              507
#define         MSG_STATE_YUYUE                 508
#define         MSG_STATE_START_CHARGE          509
#define         MSG_STATE_STOP_CHARGE           510
#define         MSG_STATE_CONTROL               511
// 后台发送命令完成
#define         MSG_FINISH_START_CHARGE         512
#define         MSG_FINISH_STOP_CHARGE          513
#define         MSG_FINISH_CHAOBIAO             514
#define         MSG_FINISH_UPDATE               515
#define         MSG_FINISH_YUYUE                516
#define         MSG_FINSIH_CONTROL              517
#define         MSG_FINISH_NO_CONTROL           518

#define         FILE_NO_EXIST                   519
#define         FILE_SIZE_ZERO                  520
#define         CHARGER_NO_EXIST                521
#define         SERVER_DEAL_ERROR               522
// finish task 链表的cmd
#define         DASH_BOARD_INFO                 1000
#define         DASH_FAST_UPDATE                1001 // 快充
#define         DASH_ERRNO_INFO                 1002
#define         DASH_FINISH_INFO                1003
#define         DASH_HB_INFO                    1004 // 用于更新和抄表进度

// 电桩命令
#define     CHARGER_CMD_NULL                0x0
#define     CHARGER_CMD_CONNECT             0x10
#define     CHARGER_CMD_CONNECT_R           0x11
#define     CHARGER_CMD_HB                  0x34
#define     CHARGER_CMD_HB_R                0x35
#define     CHARGER_CMD_CHARGE_REQ          0x54
#define     CHARGER_CMD_CHARGE_REQ_R        0x55
#define     CHARGER_CMD_STATE_UPDATE        0x56
#define     CHARGER_CMD_STOP_REQ            0x58
#define     CHARGER_CMD_STOP_REQ_R          0x57
#define     CHARGER_CMD_CTRL_R              0xa2
#define     CHARGER_CMD_CTRL                0xa3
#define     CHARGER_CMD_CHAOBIAO            0xa5
#define     CHARGER_CMD_CHAOBIAO_R          0xa4
#define     CHARGER_CMD_START_UPDATE_R      0x90
#define     CHARGER_CMD_UPDATE              0x95
#define     CHARGER_CMD_UPDATE_R            0x94
#define     CHARGER_CMD_YUYUE_R             0x41
#define     CHARGER_CMD_YUYUE               0x42
#define     CHARGER_CMD_CONFIG              0x50
#define     CHARGER_CMD_CONFIG_R            0x51
#define     CHARGER_CMD_EXCEPTION           0xee
#define     CHARGER_CMD_START_CHARGE_R      0x60
#define     CHARGER_CMD_CHARGE_ERROR        0x61
#define     CHARGER_CMD_STOP_CHARGE_R       0x62
#define     CHARGER_CMD_STOP_CHARGE         0x63
#define     CHARGER_CMD_AUTH_R              300
#define     CHARGER_CMD_AUTH                301
#define     CHARGER_CMD_BOOT                302
#define     CHARGER_CMD_CONTROL_R           303
#define     CHARGER_CMD_RECORD_R            304
#define     CHARGER_CMD_OTHER               305

#define     EVG_16N	16
#define	    EVG_32N	32

// power bar 信息
#define     POWER_BAR_PWN_1S        0x31
#define     POWER_BAR_PWN_2S        0x32
#define     POWER_BAR_PWN_3S        0x33
#define     POWER_BAR_PWN_4S        0x34
#define     POWER_BAR_PWN_5S        0x35
#define     POWER_BAR_GREEN         0x32
#define     POWER_BAR_RED           0x31

// 数据库信息,相关目录，以及文件权限
#define     CHARG_FILE	            "/etc/config/chargerinfo"
#define     SERVER_FILE             "/etc/config/chargerinfo"
#define     POWER_BAR_SERIAL        "/dev/ttyUSB0"
#define     WORK_DIR                "/mnt/umemory/power_bar"
#define     TAB_POS		            "chargerinfo.TABS.tables"	// 存放数据表的位置
#define     CHARG_FILE		        "/etc/config/chargerinfo"	
#define     CHAOBIAO_DIR            "DATA"                      // 存抄表记录文件，以CID命名
#define     UPDATE_DIR              "UPDATE"                    // 存更新包文件，以CNMB.bin命名
#define     CONFIG_DIR              "CONFIG"                    // 存推送配置文件,以CID命名
#define     RECORD_DIR              "RECORD"
#define     EXCEPTION_DIR           "exception"
#define     LOG_DIR                 "log"
#define     ROUTER_LOG_DIR          "/mnt/umemory/routerlog"
#define     CONFIG_FILE             "/etc/chargerserver.conf"
#define     FILEPERM                (S_IRUSR | S_IWUSR)
#define     MAX_LEN                 1500

/**
 *  程序维护了两个全局链表：任务等待处理命令链表和操作完成链表
 *  任务等待命令宏定义
 */
#define     WAIT_CMD_NONE               0x00
#define     WAIT_CMD_CHAOBIAO           0x01
#define     WAIT_CMD_ONE_UPDATE         0x02
#define     WAIT_CMD_CONFIG             0x03
#define     WAIT_CMD_YUYUE              0x04
#define     WAIT_CMD_ALL_UPDATE         0x05
#define     WAIT_CMD_UPLOAD             0x06    //上传充电数据
#define     WAIT_CMD_ALL_CHAOBIAO       0x07    
#define     WAIT_CMD_CTRL               0x08
#define     WAIT_CMD_START_CHARGE       0x09
#define     WAIT_CMD_STOP_CHARGE        10
#define     WAIT_CMD_WAIT_FOR_CHARGE_REQ  11
// 状态码

#define     WEB_WAY                 0x01
#define     SERVER_WAY              0x02    // 后台模式


/********************************************************************
 *
 *          服务器相关定义
 *
 ********************************************************************/

#define   SERV_PORT	        6666	// 服务器端口
#define	  LISTENMAXLEN	    100     // listen函数的backlog参数
#define MAX_LEN     2048
#define  HASH_BUCKETS           128

// 守护进程文件锁位置以及权限
#define  LOCKFILE 	"/var/run/chargerserver_daemon.pid"
#define  PIPEFILE       "/var/ev_pipe"
#define  LOCKMODE	(S_ISUID|S_ISGID|S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)


/***********************************************************************
 *
 *          相关的数据结构定义
 *
 * *********************************************************************/

// 链表接收命令的数据结构
struct cb   { time_t  start_time;   time_t  end_time;  int chargercode;                                         };
struct upt  { char    version[2];   char    name[40];                                                           };
struct cfg  { char    name[20];                                                                                 };
struct yy   { int     time;         char uid[33];                                                               };
struct ctl   {  unsigned char  value;   unsigned char status;                                                   };
struct stp { int username; char   uid[33];                                                                                    };
struct stg { char order_num[25]; char package[15]; char uid[33]; short current; short energy;                   };
// 待处理命令队列
struct  NewTask {
   unsigned  char       way;        // 页面设置方式还是服务器设置方式
   unsigned  char       cmd;
   int       cid;
   int       pipefd;
    union  {
        struct  cb   chaobiao;
        struct  upt  update;
        struct  cfg  config;
        struct  yy   yuyue;
        struct ctl  control;
        struct  stg  start_charge;
        struct  stp  stop_charge;
    }u;
};

// 管道通信的数据结构
struct PipeTask {
        int way;
        int cmd;
        int cid;
        int value;
};

struct Message {
        int             file_fd;        //打开配置文件的文件描述符，抄表描符，更新描述符
        int             file_length;
        int             file_package;   // 1024作为除数 
        int             config_frame_size;          //配置文件的包大小(文件大小/1024)
        struct NewTask  new_task;
};

// 管理充电桩信息的数据结构，动态表单，用于查找每条充电桩
typedef struct  chargerinfo{
        char            charger_type;        // 电桩类型
        char            MAC[20];
        unsigned char   present_mode;
        unsigned char   last_present_mode;
        unsigned char   target_mode;
        unsigned char   system_message;
        int             last_power;
        int             last_submode;
        int             sockfd;
        int             sequence;
        int             CID;		                // 电桩CID信息
	int             present_cmd;	            //当前接受的命令
        int             present_status;
        int             server_send_status;
        time_t          last_update_time;           // 上一次发送命令的时间
        time_t          connect_time;          // 上一次电桩联网时间
        struct   Message *message;
}CHARGER_INFO_TABLE;

// 已经处理完成命令队列
struct stop {  unsigned char  value;        };
struct  finish_task {
    int                 cmd;
    int                 cid;
    int                 errcode;
    char                *str;
    const CHARGER_INFO_TABLE  *charger;
    struct  finish_task *next;
    struct  finish_task *pre_next;
};

// 充电协议相关
//PresentMode
typedef enum {
	CHARGER_READY = 0x01,
	CHARGER_RESERVED = 0x04, 		// 预约
	CHARGER_PRE_CHARGING = 0x09,		// 预充电
	CHARGER_CHARGING_IN_QUEUE = 0x0A, 	//充电序列中
	CHARGER_CHARGING = 0x0B,
	CHARGER_CHARGING_COMPLETE = 0x0D,
	CHARGER_OUT_OF_SERVICE = 0x0E,
	CHARGER_CHARGING_COMPLETE_LOCK = 0x1E,
    CHARGER_OFF_NET = 0x2E,
	CHARGER_BOOT_UP = 0,
	CHARGER_DEBUGGING = 0x65,
	CHARGER_ESTOP = 0x66
}PresentMode;

typedef enum {
	SM_Already_Plug_Line = 0x0001,	//bit0  已插线
	SM_Is_Charging	  = 0x0002,	// bit1 正在充电
	SM_Charge_Complete  = 0x0004,	// bit2 充电完成
	SM_Have_Parking   = 0x0040,	// bit6 有泊车
	SM_Connected_Join = 0x0080,	// bit7 已连接伺服
	SM_13A_Plug	  = 0x0800,	// 13A 已插
	SM_20A_Plug	  = 0x1000,	// 20A 已插
	SM_32A_Plug	  = 0x2000,	// 32A 已插
	SM_63A_Plug	  = 0x4000,	// 63A 已插
	SM_16A_Plug	  = 0x8000	// 16A 已插
}SubMode;

typedef enum {
	SS_BS1363 	= 0x01,		
	SS_SAEJ1772 	= 0x02,
	SS_IEC62196	= 0x03,	// 1 phase
	SS_GB20234	= 0x04,	// 1 phase
	SS_IEC62196_3phase = 0x05,	// 3 phase
	SS_GB20234_3phase	= 0x06, // 3 phase
	SS_CHAde_Mo	= 0x08, // 
	SS_IEC_Combo 	= 0x09,
	SS_GB_QUICK_CHARGER	= 0x0A	// 快充
}SelectSocket; //插座


typedef enum {
	SYM_NULL			= 0x00,
	SYM_Charging_Is_Starting	= 0x01,
	SYM_Charging_Is_Stopping	= 0x02,
	SYM_No_Access_Right		= 0x03, //没有权限
	SYM_Not_Money_Charging		= 0x04, //没有足够的余额充电
	SYM_Monthly_Quata_Used_Up	= 0x05, // 每月限额已用完
	SYM_EV_Link_Not_Valid		= 0x06, // 易充卡无效
	SYM_EV_Link_Is_Used		= 0x07,	// 易充卡已经使用
        SYM_OFF_NET                     = 0xEF,
	SYM_Charger_Is_Repair		= 0xF0 // 充电桩维修
}SystemMessage;

#endif // serv_cmnfig.h


