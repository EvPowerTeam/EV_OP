#ifndef  __EV_COMMUNICATION__H
#define  __EV_COMMUNICATION__H

#include <jansson.h>
#include "serv_config.h"

#define         JSON_CMD_INDEX          2
#define         JSON_OBJECT_INDEX       3

// 替代函数
#define         protocal_get_cmd_from_json      ev_get_cmd_from_jansson
#define         protocal_decode_json            ev_decode_jansson
#define         protocal_free                   json_decref

// 协议类型定义
// key的名称与类型定义
// 命令名称与类型以及命令的处理函数定义
struct pro_command_t {
        char *name;
        int   cmd_type;
        int   (*serv) (int, json_t *, char *, CHARGER_INFO_TABLE *);
};

struct Protocal_Controller {
        struct  pro_command_t    *cmd_index;
        struct  pro_command_t    *cmd;
        struct  Protocal_Controller  *(*init)(void);
        json_t *(*parse_json)(const char *str, size_t flag);
        const char *(*get_cmd_from_json)(json_t *json, int index);
        int   (*start_run_cmd)( struct pro_command_t *, int, json_t *, char *,  CHARGER_INFO_TABLE *);
        int   (*err_report)(CHARGER_INFO_TABLE *, int );
        void  (*json_free)(json_t * json);
};

extern  struct Protocal_Controller *global_protocal_controller;

// 协议入口函数
int start_service(int fd, char *data, int cmd_type, int cid);

// 协议对象初始化
void protocal_controller_init(void);


#endif  


