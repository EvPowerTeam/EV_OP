

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
#ifndef   NDEBUG
#include <assert.h>
#define DEBUG_ERR(format,...) 	printf("FILE: "__FILE__", LINE: %d: "format"\n", __LINE__, ##__VA_ARGS__)
#define DEBUG(...)		printf(__VA_ARGS__)
#endif

typedef	 unsigned char	u8;
typedef  unsigned short u16;
typedef  unsigned int   u32;


/******************************************************
 *
 *      充电协议相关
 *
 *****************************************************/

// 错误码定义,未完善
#define     ESERVER_RESTART         0x1
#define     ESERVER_SEND_SUCCESS    0x2
#define     ESERVER_API_ERR         0x3
#define     ESERVER_CRC_ERR         0x4
// 抄表错误码
#define     ECHAOBIAO_FINISH       0x20
#define     ECHAOBIAO_API_ERR      0x21

// 更新错误码
#define     EUPDATE_FINISH          0x40
#define     EUPDATE_API_ERR         0x41    // 统一的API错误，以后可以具体详细

#define     EYUYUE_FINISH           0x50
#define     EYUYUE_API_ERR          0x51

#define     ECONFIG_FINISH          0x60
#define     ECONFIG_API_ERR         0x61

#define     ECONTROL_FINISH         0x70    // 受控
#define     ECONTROL_NO_CTRL        0x71    // 不受控
#define     ECONTROL_API_ERR        0x72
#define     ESTART_CHARGE_FINISH    0x80
#define     ESTART_CHARGE_API_ERR   0x81
#define     ESTOP_CHARGE_FINISH     0x82
#define     ESTOP_CHARGE_API_ERR    0x83

// 命令定义
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
#define     CHARGER_CMD_CONFIG              0x50
#define     CHARGER_CMD_CONFIG_R            0x51
#define     CHARGER_CMD_EXCEPTION           0xee
#define     CHARGER_CMD_START_CHARGE_R      0x60
#define     CHARGER_CMD_CHARGE_ERROR        0x61
#define     CHARGER_CMD_STOP_CHARGE_R       0x62
#define     CHARGER_CMD_STOP_CHARGE         0x63

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
#define     CONFIG_FILE             "/etc/chargerserver.conf"
#define     FILEPERM                (S_IRUSR | S_IWUSR)

/**
 *  程序维护了两个全局链表：任务等待处理命令链表和操作完成链表
 *  任务等待命令宏定义
 */
#define     WAIT_CMD_NONE           0x00
#define     WAIT_CMD_CHAOBIAO       0x01
#define     WAIT_CMD_ONE_UPDATE     0x02
#define     WAIT_CMD_CONFIG         0x03
#define     WAIT_CMD_YUYUE          0x04
#define     WAIT_CMD_ALL_UPDATE     0x05
#define     WAIT_CMD_UPLOAD         0x06    //上传充电数据
#define     WAIT_CMD_ALL_CHAOBIAO   0x07    
#define     WAIT_CMD_CTRL           0x08
#define     WAIT_CMD_START_CHARGE   0x09
#define     WAIT_CMD_STOP_CHARGE    10

#define     WEB_WAY                 0x01
#define     SERVER_WAY              0x02    // 后台模式


/********************************************************************
 *
 *          服务器相关定义
 *
 ********************************************************************/

#define   SERV_PORT	        8888	// 服务器端口
#define	  LISTENMAXLEN	    100     // listen函数的backlog参数

// 守护进程文件锁位置以及权限
#define  LOCKFILE 	"/var/run/chargerTest_daemon.pid"
#define  LOCKMODE	(S_ISUID|S_ISGID|S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)


/***********************************************************************
 *
 *          相关的数据结构定义
 *
 * *********************************************************************/

// 线程存储数据结构
typedef struct {
    unsigned char   *recv_buff;
    unsigned char   *send_buff;
    char            *val_buff;
    int             recv_cnt;
    int             ErrorCode;
}BUFF;

// 链表接收命令的数据结构
struct cb   { time_t  start_time;   time_t  end_time;  int chargercode;                                         };
struct upt  { char    version[2];   char    name[20];                                                           };
struct cfg  { char    name[20];                                                                                 };
struct yy   { int     time;         char uid[32];                                                               };
struct ctl   {  unsigned char  value;   unsigned char status;                                                   };
struct stp { char   uid[32];                                                                                    };
struct stg { char order_num[25]; char package[15]; char uid[32]; short current; short energy;                   };
// 待处理命令队列
struct  wait_task {
   unsigned  char       way;        // 页面设置方式还是服务器设置方式
   unsigned  char       cmd;
   unsigned  int        cid;
    union  {
        struct  cb   chaobiao;
        struct  upt  update;
        struct  cfg  config;
        struct  yy   yuyue;
        struct ctl  control;
        struct  stg  start_charge;
        struct  stp  stop_charge;
    }u;
    struct  wait_task   *next;
    struct  wait_task   *pre_next;
};

// 已经处理完成命令队列
struct stop {  unsigned char  value;        };
struct  finish_task {
    unsigned char       way;
    unsigned char       cmd;
    unsigned int        cid;
    int                 errcode;
    union {
        struct cb   chaobiao;
        struct ctl  control;
        struct stop stop_charge;
    }u;
    struct  finish_task *next;
    struct  finish_task *pre_next;
};

// 管理充电桩信息的数据结构，动态表单，用于查找每条充电桩
typedef struct  chargerinfo{
    char            charger_type;        // 电桩类型
    char            version[2];
    char            MAC[20];                    //mac地址，字符串
	char 	        tab_name[10];               //当前数据库表名
	char 	        is_charging_flag;		      // 电桩正在充电的标志
	unsigned char   present_cmd;	            //当前接受的命令
	unsigned char   load_balance_cmd;	            //是否有load—balance充电请求的命令
    unsigned char   way;                // WEB 命令还是后台命令
    unsigned char   wait_cmd;           // 执行后台发送的命令
    unsigned char 	IP[4];		                // 电桩IP地址
	unsigned char 	KEYB[16];		            // keyb
	unsigned char   ev_linkid[16];	            //evlink卡用户名
	unsigned char   model;		                //电桩信息，型号对应的电流大小 (EVG-32N--->32A)
	unsigned char   real_current;               //load_balance程序分配的动态电流
    unsigned char   real_time_current;
    unsigned char	free_cnt;                   //联网的计数器，当计数达到15次(30s),此电桩认为已经断网
	unsigned char	free_cnt_flag;              //联网计数器标志位，服务器接受一个信息时，设置该标志为1，联网计数器设置为0.
    unsigned char   config_frame_size;          //配置文件的包大小(文件大小/1024)
    unsigned char   support_max_current;        // 支持的最大电流
    unsigned char   target_mode;
    unsigned char   system_message;
    unsigned char   config_num;                 // 配置文件包号
    unsigned char   stop_charge_value;
    unsigned char   control_cmd;                // 控制命令值
	unsigned short  charging_code;	            //充电记录:
    unsigned short  yuyue_time;
    unsigned short  cb_target_id;
    unsigned short  cb_charging_code;
	unsigned short  power;		                //充电电量
    short           start_charge_current;       // 开始充电电流
    short            start_charge_energy;        // 电量
    char            *uid;
    char            *start_charge_order;        // 订单号
    char            *start_charge_package;      // 套餐
    int             file_length; 
    int             file_fd;                    //打开配置文件的文件描述符，抄表描符，更新描述符
    time_t          cb_start_time;        //记录服务器发送的开始抄表时间戳
    time_t          cb_end_time;          //记录服务器发送的结束抄表时间戳
    unsigned int    CID;		                // 电桩CID信息
	unsigned int    start_time; 	            // 开始充电时间
	unsigned int    end_time;	                // 结束充电时间
}CHARGER_INFO_TABLE;


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
	SYM_Charger_Is_Repair		= 0xF0 // 充电桩维修
}SystemMessage;

#endif // serv_cmnfig.h




