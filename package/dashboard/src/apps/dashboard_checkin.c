
/***
 *
 * Copyright (C) 2009-2015 Open Mesh, Inc.
 *
 * The reproduction and distribution of this software without the written
 * consent of Open-Mesh, Inc. is prohibited by the United States Copyright
 * Act and international treaties.
 *
 ***/

#include "dashboard_checkin.h"
#include "include/dashboard.h"
#include <libev/process.h>
#include <libev/const_strings.h>
#include <libev/uci.h>
#include <libev/cmd.h>
#include <libev/file.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/file.h>

static char val_buff[65], more_buff[50], env_buff[65], path_rep[50];
//static const char path_invalid_key[] = "/tmp/invalid_key";
static const char path_checkin_timeout[] = "/tmp/checkin_timeout";
static const char path_alt_checkin_timeout[] = "/tmp/alt_checkin_timeout";
static const char path_checkin_req[] = "/tmp/dashboard.%d.req";
static const char path_checkin_rep[] = "/tmp/dashboard.%d.rep";
static const char path_checkin_log[] = "/tmp/checkin.%d.log";
static char *dashboard_host, *dashboard_path, *dashboard_host_env;

/* path used when calling the checkin API */
#define API_CHECKIN_URL_FMT "test.e-chong.com:8080/ChargerAPI"
#define API_UPDATE_FMT "/ChargerState/updateState"
#define API_START_CHARGING_FTM "/Charging/canStartCharging"

#define UCI_DATABASE_NAME       "chargerinfo"
#define UCI_DATABASE_CMD_NAME   "serverinfo"
#define MAX_LEN                 255

int dashboard_checkin_now(int DASH_UNUSED(argc), char DASH_UNUSED(**argv))
{
	uint8_t checkin_parts = CHECKIN_CT | CHECKIN_PA;
	cmd_run("echo checkin now!");
	return dashboard_checkin(&checkin_parts);
}

static int _decrypt(char *data, char *key, int warn_on_err)
{
	return 0;
}

/* decrypt packet */
static int decrypt_dshbrd_data(char *fpath)
{
	return 0;
}

static void new_config(void)
{
	return;
}


#if defined(DASH_DEBUG)
int new_config_wrapper(int argc, char **argv)
{
	return 0;
}
#endif

int dashboard_checkin(void *arg)
{
	char buff[100] = "{\"chargerID\":\"123\", \"state\", 2}";
	debug_msg("performing checkin");
	api_send_buff("http", API_CHECKIN_URL_FMT, "/ChargerState/updateState", buff, NULL, NULL);

	return 0;
}

typedef struct {
    char    state;
    int     CID;
    
}CHARGER_INFO;

struct  issue_chaobiao  {    time_t start_time,  end_time;       };
struct  issue_yuyue     {           };
struct  issue_config    {           };
struct  issue_update    {           };

typedef struct {
    int          cmd;
    unsigned int cid;
    union 
    {
        struct  issue_chaobiao  chaobiao; 
        struct  issue_yuyue     yuyue;
        struct  issue_config    config;
        struct  issue_update    update;
    }cmd_param;
}SERVER_CMD;


int     json_insert_database(SERVER_CMD CMD)
{
    char    val_buff[100];

    sprintf(val_buff, "%d%c", CMD.cmd, '\0');
    if (ev_uci_save_val_string(val_buff, "%s.%s", UCI_DATABASE_CMD_NAME, "SERVER.CID") < 0)    // CMD
         goto err;
    sprintf(val_buff, "%08d%c", CMD.cid, '\0');
    if (ev_uci_save_val_string(val_buff, "%s.%s", UCI_DATABASE_CMD_NAME, "SERVER.CID") < 0)    // CID
         goto err;

    switch (CMD.cmd)
    {
        case    1:      //预约
        case    2:      //抄表
                sprintf(val_buff, "%d%c", CMD.cmd_param.chaobiao.start_time, '\0');
                if (ev_uci_save_val_string(val_buff, "%s.%s", UCI_DATABASE_CMD_NAME, "SERVER.START_TIME") < 0)    // start_time
                    goto err;
                sprintf(val_buff, "%08d%c", CMD.cmd_param.chaobiao.end_time, '\0');
                if (ev_uci_save_val_string(val_buff, "%s.%s", UCI_DATABASE_CMD_NAME, "SERVER.END_TIME") < 0)    // end_time
                    goto err;

        break;
        case    3:      //推送配置

        break;
        case    4:      //软件更新

        break;
    }

    return 0;
err:
    return -1;

}

/**
 *      func    :   char *json_query_databse(unsigned int)
 *      param   :   cid
 *      return  :   返回一个电桩信息拼接的字符串，json格式，失败返回NULL
 */

char    *json_query_database(unsigned int cid)
{
    static char spr_buff[MAX_LEN];
    int i;
    char    tab_name[10], val_buff[100], state = 0;
    CHARGER_INFO    charger_info;

    for (i = 1; i <= 8; i++)
    {
        sprintf(tab_name, "%s%d%c", "charger", i, '\0');    // 获取表名
        if (ev_uci_data_get_val(val_buff, 100, "%s.%s.%s",UCI_DATABASE_NAME, tab_name, "CID") < 0)
        {
            debug_msg("get cid error\n");
            continue;
        }
        if (cid == atoi(val_buff))
        {
            ev_uci_data_get_val(val_buff, 100, "%s.%s.%s",UCI_DATABASE_NAME, tab_name, "IP");            
            ev_uci_data_get_val(val_buff, 100, "%s.%s.%s",UCI_DATABASE_NAME, tab_name, "IP");            
            ev_uci_data_get_val(val_buff, 100, "%s.%s.%s",UCI_DATABASE_NAME, tab_name, "IP");            
            ev_uci_data_get_val(val_buff, 100, "%s.%s.%s",UCI_DATABASE_NAME, tab_name, "IP");            
            ev_uci_data_get_val(val_buff, 100, "%s.%s.%s",UCI_DATABASE_NAME, tab_name, "IP");            
            state = 1;
        }
    }
    if (state == 0)
        return NULL;
    snprintf(spr_buff, MAX_LEN,  "%s=%08d&%s=%d%c", "chargerID", charger_info.CID, "state", charger_info.state, '\0');

    return spr_buff;
}
