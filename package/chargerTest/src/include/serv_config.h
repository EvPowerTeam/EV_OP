

#ifndef __SERV_CONFIG__H
#define __SERV_CONFIG__H


#include <pthread.h>
#include <sys/types.h>
// 调试代码
//#define  	NDEBUG
#ifndef   NDEBUG
#include <assert.h>
#define DEBUG_ERR(format,...) 	printf("FILE: "__FILE__", LINE: %d: "format"\n", __LINE__, ##__VA_ARGS__)
#define DEBUG(...)		printf(__VA_ARGS__)
#endif


typedef	 unsigned char	u8;
typedef  unsigned short u16;
typedef  unsigned int   u32;

#define   SERV_PORT	8888	// 服务器端口

#define  MAXCLI		15	// 线程处理连接数

// 守护进程文件锁位置
#define  LOCKFILE 	"/var/run/EVserver_daemon.pid"
#define  LOCKMODE	(S_ISUID|S_ISGID|S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

// 数据库信息
#define  TAB_POS		"chargerinfo.TABS.tables"	// 存放数据表的位置
#define  CHARG_FILE		"etc/config/chargerinfo"	

// 充电协议配置说明
#define	CMD_0X11_LEN	31
#define CMD_0X35_LEN	12
#define CMD_0X55_LEN	34
#define	CMD_0X57_LEN	29

#define CMD_0XA2_LEN	11
#define CMD_0XA4_LEN	11
#define CMD_0X90_LEN	14
#define	CMD_0X94_LEN	0	

#define  EVG_16N	16
#define	 EVG_32N	32

typedef  struct {
	int 	fd;	// 已连接的套接字描述符
	int	cid;	//CID
unsigned int	ip;	// 客户端IP地址

}SERV_INFO;



// 线程类型定义
typedef struct {
	pthread_t 	thread_tid;
	long		thread_count;


}Thread_Type;



// 管理充电桩信息的数据结构，动态表单，用于查找每条充电桩
typedef struct {
	unsigned char  model;		
	unsigned char 	flag;		// 用于判断每台电桩的连接状态
	unsigned char  flag_cnt;
	unsigned char 	IP[4];		// 电桩IP地址
	unsigned char 	KEYB[16];		// keyb
	unsigned char 	tab_name[10];
	unsigned char   ev_linkid[16];	//evlink卡用户名
	unsigned char	charger_way;
	unsigned char	change_current_cnt;	//达到一定时间之后，进行电流改变
	unsigned char   change_current;
	char		ev_linkidtmp[50];
	unsigned short  charging_code;	//充电记录:
	unsigned int    CID;		// 电桩CID信息
	unsigned int    start_time; 	// 开始充电时间
	unsigned int    end_time;	// 结束充电时间
	unsigned short  power;		//充电电量
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



