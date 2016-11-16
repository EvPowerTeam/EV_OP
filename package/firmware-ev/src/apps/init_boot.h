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

int init_boot_boot(int argc, char **argv, char *extra_arg);
