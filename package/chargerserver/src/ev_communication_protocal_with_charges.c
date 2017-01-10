
#include <stdio.h>
#include <pthread.h>
#include "include/list.h"
#include "include/ev_lib.h"
#include "include/ev_jansson_charger.h"
#include "include/ev_communication_protocal_with_charges.h"

void err_report(int err_code);
int  doit_boot(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
int  doit_heartbeat(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
int  doit_authorize(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
int  doit_authorize_r(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_start_req(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_state_update(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_stop_req(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_start_charge_r(int fd, json_t *object, char *data, CHARGER_INFO_TBALE *charger);
//int  doit_stop_charge_r(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_control_r(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_update_r(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_config_r(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_record_r(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_message_r(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);
//int  doit_start_req_r(int fd, json_t *object, char *data, CHARGER_INFO_TABLE *charger);

struct Protocal_Controller *global_protocal_controller = NULL;
static struct pro_command_t all_serv[] = {
        {"BootNotification",    CHARGER_CMD_BOOT,               doit_boot               },
        {"Heartbeat",           CHARGER_CMD_HB,                 doit_heartbeat          },
        {"Authorize",           CHARGER_CMD_AUTH,               doit_authorize          },
        {NULL,                  CHARGER_CMD_AUTH_R,             doit_authorize_r        },
//        {"StartCharging",       CHARGER_CMD_START_REQ,          doit_start_req          }, //开始充电请求
//        {"StatusNotification",  CHARGER_CMD_STATE_UPDATE,       doit_state_update       },
//        {"StopCharging",        CHARGER_CMD_STOP_REQ,           doit_stop_req           }, //结束充电请求
//        {"RemoteStartCharging", CHARGER_CMD_START_CHARGE_R,     doit_start_charge_r     },
//        {"RemoteStopCharging",  CHARGER_CMD_STOP_CHARGE_R,      doit_stop_charge_r      },
//        {"RemoteControl",       CHARGER_CMD_CONTROL_R,          doit_control_r          },
//        {"UpdateFW",            CHARGER_CMD_UPDATE_R,           diot_update_r           },
//        {"UpdateCF",            CHARGER_CMD_CONFIG_R,           doit_config_r           },
//        {"UpdateRecord",        CHARGER_CMD_RECORD_R,           doit_record_r           },
//        {"TriggerMessage",      CHARGER_CMD_MESSAGE_R,          doit_message_r          },
        {NULL,                  0,                              NULL                    }
};

// 本文件所有函数声明
static inline struct pro_command_t *look_for_command(struct pro_command_t *ctrl, const char *cmd, int cmd_type)
{
        while (ctrl->name || ctrl->cmd_type)
        {
                if ( (cmd && !strcmp(ctrl->name, cmd)) || (cmd_type && ctrl->cmd_type == cmd_type))
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
                        if ( !(cid = json_integer_value( json_object_get(json_array_get(root, JSON_OBJECT_INDEX), "Cid") ))) {
                                printf("get cid wrong ...\n"); 
                               goto error;
                        }
                } else  //不是json数据(更新，抄表，推送配置)
                {
                        //需要解析数据
                        cid = 0;
                }
        } else  // 后台发送命令
        {
                ctrl->cmd = look_for_command(ctrl->cmd_index, NULL, cmd_type);
        }
        // 查找cid
        printf("look for cid ...\n");
        charger = look_for_charger_from_array(cid);
        // 服务程序
       if (!ctrl->cmd)
                goto error;
       err_code = (*ctrl->start_run_cmd)(ctrl->cmd, sockfd, root, data, charger);
error:
       if (ctrl->err_report)
            (*ctrl->err_report) (charger, err_code);
       
       // 释放json内存
        if (root)
               (*ctrl->json_free)(root);

       return 0;
}



// 路由跟电桩协议函数
int  doit_authorize_r(int fd, json_t *root UNUSED, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        char    *send_string;
        json_t *node[10], *array, *object;
        unsigned char node_cnt = 0;

       // 发送回应
        array = json_array();
        // msg_type
        json_array_append_new(array, (node[node_cnt++] = json_integer(3)));
        // sequence
        json_array_append_new(array, (node[node_cnt++] = json_integer(charger->sequence)));
       // command
        json_array_append_new(array, (node[node_cnt++] = json_string("Authorize")));
        // object
        object = json_object();
        //system_message
        json_object_set_new(object, "SystemMessage", node[node_cnt++] = json_integer(charger->system_message));
        json_array_append_new(array, object);

        // format string for json
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        // free json datastruct
        while (node_cnt)
                json_decref(node[--node_cnt]);
         json_decref(object);
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
        char            *send_string = NULL;
        long            power, cycle, charging_way, status, integer, new_mode;
        struct          finish_task     task;

        object = json_array_get(root, JSON_CMD_INDEX);
        // uid
        sub_object = json_object_get(object, "UidInfo");
        if ( !(uid = json_string_value(json_object_get(sub_object, "Id"))) )
                goto error;
        printf("uid:%s\n", uid);
        // chargerinfo
        sub_object = json_object_get(object, "ChargingInfo");
        // presentmode
        if ( !(json_tmp = json_is_number( json_object_get(object,  "PresentMode"))) )
            goto error;
        new_mode = json_integer_value(json_tmp);
        // starttime
        if ( !(json_tmp = json_is_number( json_object_get(object,  "StartTime"))) )
            goto error;
        integer = json_integer_value(json_tmp);
        // usetime
        if ( !(json_tmp = json_is_number( json_object_get(object,  "UseTime"))) )
            goto error;
        integer = json_integer_value(json_tmp);
        // vol
        if ( !(json_tmp = json_is_number( json_object_get(object,  "Vol"))) )
            goto error;
        integer = json_integer_value(json_tmp);
        // cur
        if ( !(json_tmp = json_is_number( json_object_get(object,  "Cur"))) )
            goto error;
        integer = json_integer_value(json_tmp);
        // power
        if ( !(json_tmp = json_is_number( json_object_get(object,  "Power"))) )
            goto error;
        power = json_integer_value(json_tmp);
        // cycle id
        if ( !(json_tmp = json_is_number( json_object_get(object,  "Cycle"))) )
            goto error;
        cycle = json_integer_value(json_tmp);
        // chargingway
        if ( !(json_tmp = json_is_number( json_object_get(object,  "ChargingWay"))) )
            goto error;
        charging_way = json_integer_value(json_tmp);
        
        if (charger->charger_type == 2 || charger->charger_type == 4)
        {
                // soc
                if ( !(json_tmp = json_is_number( json_object_get(object,  "Soc"))) )
                        goto error;
                integer = json_integer_value(json_tmp);
                // remaintime
                if ( !(json_tmp = json_is_number( json_object_get(object,  "RemainTime"))) )
                        goto error;
        integer = json_integer_value(json_tmp);
                if (power - charger->last_power >= 1000)
                {
                        // 发送命令
                        task.cmd = WAIT_CMD_UPDATE_FAST_API;
                        task.cid = charger->CID;
                        finish_task_add(SERVER_WAY, &task);
                }
        }
        if (charger->present_mode != new_mode && charger->present_mode > 0)
        {
                if ( (send_string = (char *)malloc(528)) )
                {
                        sprintf(send_string, 
                        "/ChargerState/stopState?key={chargers:[{chargerId:\\\"%08d\\\",privateID:\\\"%s\\\",power:%ld,chargingRecord:%ld,mac:\\\"%s\\\",chargingType:%ld,status:%ld}]}", 
                        charger->CID, uid, power, cycle, charger->MAC, charging_way, new_mode);
                        task.str = send_string;
                        task.cid = charger->CID;
                        task.cmd = WAIT_CMD_STOP_STATE_API;
                        finish_task_add(SERVER_WAY, &task);
                }
        }
        charger->present_mode = new_mode;
       // 发送回应
        json_t *node[10], *array; unsigned char node_cnt = 0;
        array = json_array();
        // msg_type
        json_array_append_new(array, (node[node_cnt++] = json_integer(3)));
        // sequence
        json_array_append_new(array, json_array_get(root, 1));
       // command
        json_array_append_new(array, (node[node_cnt++] = json_string("StatusNotification")));
        // object
        object = json_object();
        json_array_append_new(array, object);

        // format string for json
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        // free json datastruct
        while (node_cnt)
                json_decref(node[--node_cnt]);
         json_decref(array);
         json_decref(object);
        // 发送数据
       writen(fd, send_string, strlen(send_string));
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
        struct join_wait_task wait_task;
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
        if (!sptr)
            wait_task.value = 0xEE;
        else
            wait_task.value = sptr[8] - '0';
        wait_task.cid = user->cid;
        wait_task.cmd = WAIT_CMD_WAIT_FOR_CHARGE_REQ;
        write(pipefd, &wait_task, sizeof(struct join_wait_task));
        free(user->info);
        free(user);
        if (sptr)
            free(sptr);
        close(pipefd);
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
        // 启动线程
        if (pthread_create(&tid, NULL, pthread_send_charger_req, user) != 0)
             goto error;

        return 0;
error:
        if (send_string)
            free(send_string);
        if (user)
            free(user);
        if (send_string)
            free(send_string);

        return -1;
}

int  doit_heartbeat(int fd, json_t *root, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t          *object, *json_tmp;
        const char      *string;
        char            *send_string;
        long            integer;

//        object = protocal_get_object(root);
        object = json_array_get(root, JSON_OBJECT_INDEX);
        // cid
        if ( !(json_tmp = json_is_number(json_object_get(object, "Cid"))) )
             goto error;
        integer = json_integer_value( json_tmp);
        printf("cid:%ld\n", integer);
        // present
        if ( !(json_tmp = json_is_number(json_object_get(object, "PresentMode"))) )
             goto error;
        integer = json_integer_value( json_tmp);
        printf("presentmode:%ld\n", integer); 
        // submode
        if ( !(json_tmp = json_is_number(json_object_get(object, "SubMode"))) )
             goto error;
        integer = json_integer_value( json_tmp);
        printf("submode:%ld\n", integer); 
       // 发送回应
        json_t *node[10], *array; unsigned char node_cnt = 0;
        array = json_array();
        // msg_type
        json_array_append_new(array, (node[node_cnt++] = json_integer(3)));
        // sequence
        json_array_append_new(array, json_array_get(root, 1));
       // command
        json_array_append_new(array, (node[node_cnt++] = json_string("Heartbeat")));
        // object
//        object = json_object();
//        json_array_append_new(array, object);
        // format string for json
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        while (node_cnt)
                json_decref(node[--node_cnt]);
        json_decref(array);
        json_decref(object);
        // 发送数据
        writen(fd, send_string, strlen(send_string));
        free(send_string); 
        return 0;
error:
        return -1;
}


int  doit_boot(int fd, json_t *root, char *data UNUSED, CHARGER_INFO_TABLE *charger)
{
        json_t          *object, *json_tmp;
        const char      *serial, *model, *ip, *mac;
        char            *send_string;
        long            integer;
        unsigned char   charger_count;

        printf("boot ...\n"); 
//        object = protocal_get_object(root);
        object = json_array_get(root, JSON_OBJECT_INDEX);
        // cid 
        if ( !(json_tmp = json_is_number(json_object_get(object, "Cid"))))
             goto error;
        integer = json_integer_value( json_tmp);
        printf("cid:%ld\n", integer);
        if (charger)
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
//        if ( !(mac = json_string_value( json_object_get(object, "Mac"))) )
//       printf("mac:%s\n", mac);
        if (charger) {
                // 找到的cid
        } else {
                // 没有的cid
        }
        json_t *node[10], *array; unsigned char node_cnt = 0;
        array = json_array();
        // msg_type
        json_array_append_new(array, (node[node_cnt++] = json_integer(3)));
        // sequence
        json_array_append_new(array, json_array_get(root, 1));
       // command
        json_array_append_new(array, (node[node_cnt++] = json_string("BootNotification")));
        // object
        object = json_object();
        json_array_append_new(array, object);

        // format string for json
        send_string = json_dumps(array, 0);
        printf("send_string:%s\n", send_string);
        // free json datastruct
        while (node_cnt)
                json_decref(node[--node_cnt]);
         json_decref(array);
         json_decref(object);
        // 发送数据
       writen(fd, send_string, strlen(send_string));
       free(send_string); 
       return 0;
error:
       return -1;
}
//const char *protocal_get_cmd_from_json(json_t *root, int cmd_index)
//{
//        return (json_string_value(json_array_get(root, cmd_index)));
//}
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


