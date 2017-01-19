
#include <stdio.h>
#include <pthread.h>
#include "include/list.h"
#include "include/ev_lib.h"
#include "include/ev_hash.h"
#include "include/ev_jansson_charger.h"
#include "include/ev_communication_protocal_with_charges.h"

void err_report(int err_code);
int  doit_boot(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_heartbeat(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_authorize(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_authorize_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_start_req(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
int  doit_state_update(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_stop_req(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_start_charge_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_stop_charge_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_control_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_update_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_config_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_record_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
int  doit_other(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_message_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_start_req_r(int fd, json_t *root, char *data, CHARGER_INFO_TABLE *charger);

static void  report_chargers_status_changes(CHARGER_INFO_TABLE *charger, const char *uid);
static int  check_doit_other(int fd, const char *data);

struct Protocal_Controller *global_protocal_controller = NULL;
static struct pro_command_t all_serv[] = {
        {"BootNotification",    CHARGER_CMD_BOOT,               doit_boot               },
        {"Heartbeat",           CHARGER_CMD_HB,                 doit_heartbeat          },
        {"Authorize",           CHARGER_CMD_AUTH,               doit_authorize          },
        {NULL,                  CHARGER_CMD_AUTH_R,             doit_authorize_r        },
//        {"StartCharging",       CHARGER_CMD_START_REQ,          doit_start_req          }, //开始充电请求
        {"StatusNotification",  CHARGER_CMD_STATE_UPDATE,       doit_state_update       },
        {"StopCharging",        CHARGER_CMD_STOP_REQ,           doit_stop_req           }, //结束充电请求
        {"RemoteStartCharging", CHARGER_CMD_START_CHARGE_R,     doit_start_charge_r     },
        {"RemoteStopCharging",  CHARGER_CMD_STOP_CHARGE_R,      doit_stop_charge_r      },
        {"RemoteControl",       CHARGER_CMD_CONTROL_R,          doit_control_r          },
        {"UpdateFW",            CHARGER_CMD_UPDATE_R,           doit_update_r           },
        {"UpdateCF",            CHARGER_CMD_CONFIG_R,           doit_config_r           },
        {"UpdateRecord",        CHARGER_CMD_RECORD_R,           doit_record_r           },
        {"NULL",                CHARGER_CMD_OTHER,              doit_other              },
//        {"TriggerMessage",      CHARGER_CMD_MESSAGE_R,          doit_message_r          },
        {NULL,                  0,                              NULL                    }
};

// 本文件所有函数声明
static inline struct pro_command_t *look_for_command(struct pro_command_t *ctrl, const char *cmd, int cmd_type)
{
        while (ctrl->name || ctrl->cmd_type)
        {
                if ( (cmd && ctrl->name && !strcmp(ctrl->name, cmd)) || (cmd_type && ctrl->cmd_type == cmd_type))
                {
                        printf("cmd found ...\n");
                        return  ctrl;
                }
                ctrl++;
        }

        return (struct pro_command_t *)NULL;
}

static int protocal_start_run_cmd(struct pro_command_t *cmd, int fd, json_t *json, char *data, CHARGER_INFO_TABLE *charger)
{
       return  (*cmd->serv)(fd, json, data, charger);
}

int start_service(int sockfd, char *data, int cmd_type, int cid)
{
        struct Protocal_Controller *ctrl = global_protocal_controller;
        json_t  *root = NULL;
        const char    *getcmd;
        int     err_code;
        CHARGER_INFO_TABLE *charger;

        printf("data:%s\n", data);
        if (data) 
        {
                // json
                root = (*ctrl->parse_json)(data, 0);
                if ( root )
                {
                        printf("json parse success ...\n");
                        if ( !(getcmd = (*ctrl->get_cmd_from_json)(root, JSON_CMD_INDEX)) ) 
                                goto error;
                        printf("getcmd:%s\n", getcmd);
                        ctrl->cmd = look_for_command(ctrl->cmd_index, getcmd, 0);
                        if ( !(cid = json_integer_value( (json_object_get(json_array_get(root, JSON_OBJECT_INDEX), "Cid")))) ) {
                                printf("get cid wrong ...\n"); 
                               goto error;
                        }
                } else  //不是json数据(更新，抄表，推送配置)
                {
                        //需要解析数据
                        if ( (cid = check_doit_other(sockfd, data)) < 0)
                                return -1;
                        ctrl->cmd = look_for_command(ctrl->cmd_index, NULL, CHARGER_CMD_OTHER);
                }
        } else  // 后台发送命令
        {
                ctrl->cmd = look_for_command(ctrl->cmd_index, NULL, cmd_type);
        }
        // 查找cid
        charger = look_for_charger_from_array(cid);
        if (charger)
                printf("look for cid end--->charger->cid:%d\n", charger->CID);
        else
                printf("look for cid end--->no charger->cid\n");
        if ( charger) {
             charger->sockfd = sockfd;
             charger->connect_time = time(0);
        }
        printf("a1 ...\n");
        // 服务程序
       if (!ctrl->cmd)
                goto error;
        printf("a2 ...\n");
       err_code = (*ctrl->start_run_cmd)(ctrl->cmd, sockfd, root, data, charger);
        printf("a3 ...\n");
error:
       if (ctrl->err_report)
            (*ctrl->err_report) (charger, err_code);
        printf("a4 ...\n");
       
       // 释放json内存
        if (root) {
               (*ctrl->json_free)(root);
        }
        printf("a5 ...\n");

       return 0;
}



// 路由跟电桩协议函数
//
int  doit_other(int fd, json_t *root UNUSED, char *data, CHARGER_INFO_TABLE *charger)
{

}
static int  check_doit_other(int fd, const char *data)
{
        // 头比较
        int cid = 0;
        if ( strncmp(data, "EV>", 3) || (data[5] != 0x95 && data[5] != 0x50 && data[5] != 0xa5) ) {
                writen(fd, "unknow data: ",  13);
                writen(fd, data,  strlen(data));
                return -1;
        }

        // 更新
        if ( data[5] == 0x95)
        {
        
        } else if ( data[5] == 0x50) // 推送配置
        {
         
        } else if (data[5] == 0xa5) // 抄表
        {
        
        
        } 
        
        return cid;
}

int  doit_record_r(int fd, json_t *root UNUSED, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t *array, *object;
        char *send_string;
        
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(2));
        // seq--> 0
        json_array_append_new(array, json_integer(0));
        // cmd
        json_array_append_new(array, json_string("UpdateRecord"));
        object = json_object();
        json_object_set_new(object, "StartTime", json_integer(charger->message->new_task.u.chaobiao.start_time));
        json_object_set_new(object, "EndTime", json_integer(charger->message->new_task.u.chaobiao.end_time));
        json_array_append_new(array, object);
        send_string = json_dumps(array, 0);
        json_decref(array);
        json_decref(object);
        writen(fd, send_string, strlen(send_string));
        free(send_string);

        return 0;
}

int  doit_config_r(int fd, json_t *root UNUSED, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t *array, *object;
        char *send_string;
        
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(2));
        // seq--> 0
        json_array_append_new(array, json_integer(0));
        // cmd
        json_array_append_new(array, json_string("UpdateCF"));
        object = json_object();
        json_object_set_new(object, "FileSize", json_integer(charger->message->file_length));
        json_object_set_new(object, "FilePackage", json_integer(charger->message->file_package));
        json_array_append_new(array, object);
        send_string = json_dumps(array, 0);
        json_decref(array);
        json_decref(object);
        writen(fd, send_string, strlen(send_string));
        free(send_string);

        return 0;
}

int  doit_update_r(int fd, json_t *root UNUSED, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t *array, *object;
        char *send_string, tmp[10];
        
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(2));
        // seq--> 0
        json_array_append_new(array, json_integer(0));
        // cmd
        json_array_append_new(array, json_string("UpdateFW"));
        object = json_object();
        sprintf(tmp, "%d.%d", charger->message->new_task.u.update.version[0], charger->message->new_task.u.update.version[1]);
        json_object_set_new(object, "FileVersion", json_string(tmp));
        json_object_set_new(object, "FileSize", json_integer(charger->message->file_length));
        json_object_set_new(object, "FilePackage", json_integer(charger->message->file_package));
        json_array_append_new(array, object);
        send_string = json_dumps(array, 0);
        json_decref(array);
        json_decref(object);
        writen(fd, send_string, strlen(send_string));
        free(send_string);

        return 0;
}

int  doit_control_r(int fd, json_t *root UNUSED, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t *array, *object;
        char *send_string;
        
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(2));
        // seq--> 0
        json_array_append_new(array, json_integer(0));
        // cmd
        json_array_append_new(array, json_string("RemoteControl"));
        object = json_object();
        if (charger->message->new_task.u.control.value == 1)
                json_object_set_new(object, "Control", json_string("restart"));
         else  if (charger->message->new_task.u.control.value == 2)
                json_object_set_new(object, "Control", json_string("floor_look_up"));
        else   if (charger->message->new_task.u.control.value == 3)
                json_object_set_new(object, "Control", json_string("floor_look_down"));
        
        json_array_append_new(array, object);
        send_string = json_dumps(array, 0);
        json_decref(array);
        json_decref(object);
        printf("send_string:%s\n", send_string);
        writen(fd, send_string, strlen(send_string));
        free(send_string);
        return 0; 
}
int  doit_stop_charge_r(int fd, json_t *root UNUSED, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t *array, *object;
        char *send_string;
        
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(2));
        // seq--> 0
        json_array_append_new(array, json_integer(0));
        // cmd
        json_array_append_new(array, json_string("RemoteStopCharging"));
        object = json_object();
        // amdin
        if (charger->message->new_task.u.stop_charge.username == 1)
                json_object_set_new(object, "User", json_string("admin"));
         else 
                json_object_set_new(object, "User", json_string(charger->message->new_task.u.stop_charge.uid));
        json_array_append_new(array, object);
        send_string = json_dumps(array, 0);
        json_decref(array);
        json_decref(object);
        printf("send_string:%s\n", send_string);
        writen(fd, send_string, strlen(send_string));
        free(send_string);

        return 0;
}

int  doit_start_charge_r(int fd, json_t *root UNUSED, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t *array, *object;
        char *send_string;
        // msg_type
        json_array_append_new(array, json_integer(2));
        // seq--> 0
        json_array_append_new(array, json_integer(0));
        // cmd
        json_array_append_new(array, json_string("RemoteStartCharging"));
        object = json_object();
        json_object_set_new(object, "Uid", json_string(charger->message->new_task.u.start_charge.uid));
        json_object_set_new(object, "Energy", json_integer(charger->message->new_task.u.start_charge.energy));
        json_array_append_new(array, object);
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        json_decref(array);
        json_decref(object);
        writen(fd, send_string, strlen(send_string));
        free(send_string);

        return 0;
}
int  doit_stop_req(int fd, json_t *root, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t  *array, *object, *uid_object, *charging_object, *json_tmp;
        long  integer, cycle, power, present_mode, acc_power, type;
        char *uid, *mac;

        if ( !charger)
                goto error;
        object = json_array_get(root, JSON_OBJECT_INDEX);
        uid_object = json_object_get(object, "UidInfo");
        charging_object = json_object_get(object, "ChargingInfo");
        // uid
        uid = json_string_value(json_object_get(uid_object, "Id"));
        // cycle
        if ( !json_is_number(json_tmp = json_object_get(object, "Cycle")) )
            goto error;
        cycle = json_integer_value(json_tmp);
        // present_mode
        if ( !json_is_number(json_tmp = json_object_get(object, "PresentMode")) )
            goto error;
        present_mode = json_integer_value(json_tmp);
        // power
        if ( !json_is_number(json_tmp = json_object_get(charging_object, "Power")) )
            goto error;
        power = json_integer_value(json_tmp);
//        // Accpower
//        if ( !json_is_number(json_tmp = json_object_get(object, "AccPower")) )
//            goto error;
//        acc_power = json_integer_value(json_tmp);
        
        // 更新charger
        char *send_string = (char *)malloc(528);
       if ( !send_string)
                goto error;
        sprintf(send_string, "/Charger/State/stopState?key={chargers:[{chargerId\\\"%08d\\\",privateID:\\\"%s\\\",power:%ld,chargingRecord:%ld,mac:\\\"%s\\\",chargingType:%ld,status:%ld}]}", 
                          charger->CID, uid, power, cycle, charger->MAC, type, present_mode);
        
        finish_task_add(DASH_BOARD_INFO, charger, send_string, 0);
        
        charger->present_mode = present_mode;
        report_chargers_status_changes(charger, uid);
        
        // 发送数据
        array = json_array();
        json_array_append_new(array, json_integer(3));
        json_array_append_new(array, (json_incref(json_array_get(root, 1))));
        json_array_append_new(array, json_string("StopCharging"));
        object = json_object();
        json_array_append_new(array, object);
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        json_decref(array);

        // 发送数据
        writen(fd, send_string, strlen(send_string));
        free(send_string);
        return 0;
error: 
        return -1;
}

int  doit_authorize_r(int fd, json_t *root UNUSED, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        char    *send_string;
        json_t  *array, *object;
        
        printf("authorize_r--->sequence:%d\n", charger->sequence);

       // 发送回应
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(3));
        // sequence
        json_array_append_new(array, json_integer(charger->sequence));
       // command
        json_array_append_new(array, json_string("Authorize"));
        // object
        object = json_object();
        //system_message
        json_object_set_new(object, "SystemMessage", json_integer(charger->system_message));
        json_array_append_new(array, object);

        // format string for json
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        // free json datastruct
        json_decref(array);
        // 发送数据
       writen(fd, send_string, strlen(send_string));
       free(send_string); 
        return 0;
        
}

int  doit_state_update(int fd, json_t *root, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t          *object, *sub_object, *json_tmp;
        const char      *string, *uid;
        char            *send_string = NULL, tmp[20];
        long            acc_power, power, cycle, charging_way, status, integer, new_mode, current, voltage, start_time, use_time;
        
        if ( !charger)
                goto error;
        object = json_array_get(root, JSON_OBJECT_INDEX);
        
        // presentmode
        if ( !json_is_number(json_tmp = json_object_get(object, "PresentMode")) )
            goto error;
        new_mode = json_integer_value(json_tmp);
        // chargingway
        if ( !json_is_number(json_tmp = json_object_get(object, "ChargingWay")) )
            goto error;
        charging_way = json_integer_value(json_tmp);
        // cycle id
        if ( !json_is_number(json_tmp = json_object_get(object, "Cycle")) )
            goto error;
        cycle = json_integer_value(json_tmp);
        // acc_power
        if ( !json_is_number(json_tmp = json_object_get(object, "AccPower")) )
            goto error;
        acc_power = json_integer_value(json_tmp);

        // uid
        sub_object = json_object_get(object, "UidInfo");
        if ( !(uid = json_string_value(json_object_get(sub_object, "Id"))) )
                goto error;
        printf("uid:%s\n", uid);
        // chargerinfo
        sub_object = json_object_get(object, "ChargingInfo");
        // power
        if ( !json_is_number(json_tmp = json_object_get(sub_object, "Power")) )
            goto error;
        power = json_integer_value(json_tmp);
        // starttime
        if ( !json_is_number(json_tmp = json_object_get(sub_object, "StartTime")) )
            goto error;
        start_time = json_integer_value(json_tmp);
        // usetime
        if ( !json_is_number(json_tmp = json_object_get(sub_object, "UseTime")) )
            goto error;
        use_time = json_integer_value(json_tmp);
        // vol
        if ( !json_is_number(json_tmp = json_object_get(sub_object, "Voltage")) )
            goto error;
        voltage = json_integer_value(json_tmp);
        // cur
        if ( !json_is_number(json_tmp = json_object_get(sub_object, "Current")) )
            goto error;
        current = json_integer_value(json_tmp);
        

        if (charger->charger_type == 2 || charger->charger_type == 4)
        {
                // soc
                if ( !json_is_number(json_tmp = json_object_get(sub_object, "Soc")) )
                        goto error;
                integer = json_integer_value(json_tmp);
                sprintf(tmp, "%ld", integer);
                ev_uci_save_val_string(tmp, "chargerinfo.%08d.Soc", charger->CID);
                // remaintime
                if ( !json_is_number(json_tmp = json_object_get(sub_object, "RemainTime")) )
                        goto error;
                integer = json_integer_value(json_tmp);
                sprintf(tmp, "%ld", integer);
                ev_uci_save_val_string(tmp, "chargerinfo.%08d.RemainTime", charger->CID);
                if (power - charger->last_power >= 1000)
                {
                        finish_task_add(DASH_FAST_UPDATE, charger, NULL, 0);
                       
                }
                charger->last_power = power;
        }
        charger->present_mode = new_mode;
        if (new_mode != charger->last_present_mode && new_mode == CHARGER_CHARGING )
        {
                if ( (send_string = (char *)malloc(528)) )
                {
                        sprintf(send_string, 
                        "/ChargerState/stopState?key={chargers:[{chargerId:\\\"%08d\\\",privateID:\\\"%s\\\",power:%ld,chargingRecord:%ld,mac:\\\"%s\\\",chargingType:%ld,status:%ld}]}", 
                        charger->CID, uid, power, cycle, charger->MAC, charging_way, new_mode);
                        finish_task_add(DASH_BOARD_INFO, charger, send_string, 0);
                }
                report_chargers_status_changes(charger, uid);
        }
        // 写入UCI
        sprintf(tmp, "%ld", new_mode);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.PresentMode", charger->CID);
        sprintf(tmp, "%ld", acc_power);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.AccPower", charger->CID);
        sprintf(tmp, "%ld", charging_way);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.ChargingType", charger->CID);
        sprintf(tmp, "%ld", cycle);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.ChargingCode", charger->CID);
        sprintf(tmp, "%ld", start_time);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.StartTime", charger->CID);
        sprintf(tmp, "%ld", use_time);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.UseTime", charger->CID);
        sprintf(tmp, "%ld", voltage);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.Voltage", charger->CID);
        sprintf(tmp, "%ld", current);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.Current", charger->CID);
        sprintf(tmp, "%ld", power);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.Power", charger->CID);
        ev_uci_save_val_string(uid, "chargerinfo.%08d.PrivateID", charger->CID);

        // 更新内存数据
       // 发送回应
       json_t *array;
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(3));
        // sequence
        json_array_append_new(array, (json_incref(json_array_get(root, 1))));
       // command
        json_array_append_new(array, json_string("StatusNotification"));
        // object
        object = json_object();
        json_array_append_new(array, object);

        // format string for json
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        // free json datastruct
         json_decref(array);
        // 发送数据
       writen(fd, send_string, strlen(send_string));
       if (send_string)
                free(send_string); 
        return 0;
error:
        return -1;        
}
struct User {
        int cid;
        char *info;
};
void *pthread_send_charger_req(void *arg)
{
        char  *sptr;
        int pipefd, i;
        struct PipeTask pipe_task;
        struct User *user = (struct User *)arg;
        
        pthread_detach(pthread_self());
        if ( (pipefd = open(PIPEFILE, O_WRONLY, 0200)) < 0)
                goto error;
        for ( i = 0; i < 3; i++)
        {
                cmd_frun("dashboard url_post %s %s", sys_checkin_url(), user->info);
                sptr = mqreceive_timed("/dashboard.checkin", 10, 3);
                if (sptr != NULL){
                        if ( !strncmp(sptr, "charging", 8))
                            break;
                        free(sptr);
                }
        }
        if (i >= 3)
             sptr = NULL;
        // 反馈的信息写入管道
        printf("phtead receive data:%s\n", sptr);
        if (!sptr)
            pipe_task.value = 0xEE;
        else
            pipe_task.value = sptr[8] - '0';
        pipe_task.cid = user->cid;
        pipe_task.cmd = WAIT_CMD_WAIT_FOR_CHARGE_REQ;
        writen(pipefd, &pipe_task, sizeof(struct PipeTask));
        free(user->info);
        free(user);
        if (sptr)
            free(sptr);
        close(pipefd);
        return NULL;
error:
        free(user->info);
        free(user);
        return NULL;
}
int  doit_authorize(int fd UNUSED, json_t *root, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t          *object, *uid_object;
        const char      *string;
        char            *send_string = NULL;
        long            integer;
        pthread_t       tid;
        struct User     *user = NULL;

//        object = protocal_get_object(root);
        object = json_array_get(root, JSON_OBJECT_INDEX);
        // 保存序列号
        charger->sequence = json_integer_value(json_array_get(root, 1));
        printf("authorize--->sequence:%d\n", charger->sequence);
        if ( !(user = (struct User *)malloc( sizeof(struct User))) )
             goto error;
        if ( !(send_string = (char *)malloc(256)))
             goto error;
        // uid 
        uid_object = json_object_get(object, "UidInfo");
        if ( !(string = json_string_value( json_object_get(uid_object, "Id"))) )
            goto error;
        sprintf(send_string, "/Charging/canStartCharging?chargerID=%08d'&'privateID=%s", charger->CID, string);
        printf("send_string:%s\n", send_string);
        user->cid = charger->CID;
        user->info = send_string;
        // 更新charger
        charger->last_power = 0;
        // 启动线程
        if (pthread_create(&tid, NULL, pthread_send_charger_req, user) != 0)
             goto error;

        return 0;
error:
        if (send_string)
            free(send_string);
        if (user)
            free(user);

        return -1;
}

int  doit_heartbeat(int fd, json_t *root, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t          *object, *json_tmp;
        const char      *string, tmp[50];
        char            *send_string;
        long            integer;

        if ( !charger)
                goto error;
        
        object = json_array_get(root, JSON_OBJECT_INDEX);
        // present
        if ( !json_is_number(json_tmp = json_object_get(object, "PresentMode")) )
                goto error;
        sprintf(tmp, "%d", (charger->present_mode = json_integer_value( json_tmp)) );
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.PresentMode", charger->CID);
        // submode
        if ( !json_is_number(json_tmp = json_object_get(object, "SubMode")) )
             goto error;
        sprintf(tmp, "%d", json_integer_value( json_tmp));
        report_chargers_status_changes(charger, NULL);
        ev_uci_save_val_string(tmp, "chargerinfo.%08d.SubMode", charger->CID);
       
        // 发送回应
        json_t *array; 
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(3));
        // sequence
        json_array_append_new(array, (json_incref(json_array_get(root, 1))));
//        json_array_append_new(array, json_array_get(root, 1));
       // command
        json_array_append_new(array, json_string("Heartbeat"));
        // object
        object = json_object();
        json_array_append_new(array, object);
        // format string for json
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        json_decref(array);
        // 发送数据
        writen(fd, send_string, strlen(send_string));
        if (send_string)
                free(send_string); 
        printf("HeartBeat cid[%d] ...\n", charger->CID);
        return 0;
error:
        return -1;
}

int  uci_exec(const char *name, void *arg)
{
        char *str = (char *)arg;
        char *sn, *sa;
      
       printf("name:%s, arg:%s\n", name, str); 
        if ( !strcmp(name, str))
             return 0;
        sn  = strchr(name, '@');
        sa =  strchr(str, '@');
        // 比较 mac
        if ( !strcmp(sn+1, sa+1) )
        {
             // 删除该list，并增加（更新）list, tmp
              ev_uci_del_list(name, "chargerinfo.chargers.cids");
              *sa = 0;
              ev_uci_add_named_sec("chargerinfo.%s=chargerinfo", str);
              *sa = '@';
              ev_uci_add_list(str, "chargerinfo.chargers.cids");
              return 0;
        }
        return -1;
}


int  doit_boot(int fd, json_t *root, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t          *object;
        const json_t    *json_tmp;
        const char      *serial, *model, *ip, *mac;
        char            *send_string, *string, *uci_value;
        long            integer, cid;
        unsigned char   charger_count;
        char            tmp[30];
        int             ret;

        printf("boot ...\n"); 
        object = json_array_get(root, JSON_OBJECT_INDEX);
        // cid 
        if ( !json_is_number(json_tmp = json_object_get(object, "Cid")) )
             goto error;
        printf("boot1 ...\n"); 
        cid = json_integer_value( json_tmp);
        printf("cid:%ld\n", cid);
        if ( !(mac = json_string_value( json_object_get(object, "Mac"))) )
       printf("mac:%s\n", mac);
        
       // series
        if ( !(serial = json_string_value( json_object_get(object, "ChargerSerial"))) )
            goto error;
       printf("serial:%s\n", serial); 
       //  model 
        if ( !(model = json_string_value( json_object_get(object, "ChargerModel"))) )
            goto error;
       printf("model:%s\n", model);
       //  localIP 
//        if ( !(ip = json_string_value( json_object_get(object, "LocalIP"))) )
//            goto error;
//       printf("localIP:%s\n", ip);
       // 发送回应
        sprintf(tmp, "%08d@%s", cid, mac);
        
        // 查找UCI中list列表;
        ret = ev_uci_list_foreach("chargerinfo.chargers.cids", uci_exec, tmp);
       printf("ret = %d\n", ret);
        // 增加新的list
        if (ret  == 0 || ret == 1)
        {
             printf("tmp:%s\n", tmp);
             ev_uci_add_list(tmp, "chargerinfo.chargers.cids");
             ev_uci_add_named_sec("chargerinfo.%08d=chargerinfo", cid);
        } else if (ret < 0)
             goto error;
        // 更新uci数据库
        sprintf(tmp, "%08d", cid);
        if (ev_uci_save_val_string(tmp, "chargerinfo.%08d.CID", cid) < 1)
             goto uci_wrong;
        if (ev_uci_save_val_string(serial, "chargerinfo.%08d.Serial", cid) < 1)
             goto uci_wrong;
        if (ev_uci_save_val_string(model, "chargerinfo.%08d.Model", cid) < 1)
             goto uci_wrong;
        if (ev_uci_save_val_string(mac, "chargerinfo.%08d.MAC", cid) < 1)
             goto uci_wrong;
//        if (ev_uci_save_val_string(ip, "chargerinfo.%08d.IP", cid) < 1)
//             goto uci_wrong;
        // 加入hash
        if ( ret == 0 || ret == 1 || !charger)
        {
                printf("add hash ...\n");
                CHARGER_INFO_TABLE chr;
                chr.CID = cid;
                strcpy(chr.MAC, mac);
                hash_add_entry(global_cid_hash, &cid, sizeof(cid), &chr, sizeof(chr));
        }
        json_t *array; 
        array = json_array();
        // msg_type
        json_array_append_new(array, json_integer(3));
        // sequence
        json_array_append_new(array, (json_incref(json_array_get(root, 1))));
       // command
        json_array_append_new(array, json_string("BootNotification"));
        // object
        object = json_object();
        json_array_append_new(array, object);

        // format string for json
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        // free json datastruct
         json_decref(array);
//        // 发送数据
       writen(fd, send_string, strlen(send_string));
       if (send_string)
                free(send_string); 
       return 0;
uci_wrong:
       // 删除uci list ，chargerinfo
error:
       return -1;
}

static void  report_chargers_status_changes(CHARGER_INFO_TABLE *charger, const char *uid)
{
        char *str;
       
        if (charger->last_present_mode == charger->present_mode)
                return ;
        if (charger->last_present_mode == 0) {
                charger->last_present_mode = charger->present_mode;
                return ;
        }
        if ( !(str = (char *)malloc(128)) )
                return ;

        if (uid)
        {
                sprintf(str, "/ChargerState/changesStatus?key={cid:\\\"%08d\\\",pid:\\\"%s\\\",status:%d}", 
                      charger->CID, uid, charger->present_mode);
        } else
        {
                sprintf(str, "/ChargerState/changesStatus?key={cid:\\\"%08d\\\",status:%d}", 
                      charger->CID, charger->present_mode);
        }
        if (charger->present_mode == CHARGER_READY)
        {
                ev_uci_delete("chargerinfo.%08d.PrivateID", charger->CID);
                ev_uci_delete("chargerinfo.%08d.ChargingCode", charger->CID);
                ev_uci_delete("chargerinfo.%08d.Voltage", charger->CID);
                ev_uci_delete("chargerinfo.%08d.Current", charger->CID);
                ev_uci_delete("chargerinfo.%08d.ChargingWay", charger->CID);
                ev_uci_delete("chargerinfo.%08d.Power", charger->CID);
                if (charger->charger_type == 2 || charger->charger_type == 4) {
                        ev_uci_delete("chargerinfo.%08d.Soc", charger->CID);
                        ev_uci_delete("chargerinfo.%08d.RemainTime", charger->CID);
                }
        }
        finish_task_add(DASH_BOARD_INFO, charger, str, 0);
        charger->last_present_mode = charger->present_mode;

        return ;
}

void protocal_controller_init(void)
{
        struct Protocal_Controller *controller;

        if ( (controller = (struct Protocal_Controller *)malloc(sizeof(struct Protocal_Controller))) == NULL)
                exit(1);
        controller->cmd_index = all_serv;
        controller->init  = NULL;
        controller->parse_json = protocal_decode_json;
        controller->get_cmd_from_json = protocal_get_cmd_from_json;
        controller->start_run_cmd = protocal_start_run_cmd;
        controller->err_report = NULL;
        controller->json_free =  protocal_free;

       global_protocal_controller = controller;
}


