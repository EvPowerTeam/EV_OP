
#include "include/file.h"
#include "include/ev_command_file.h"
#include "include/ev_charger_l.h"
#include "include/err.h"
#include <stdio.h>
#include  <string.h>
extern int opt_type;

int write_file(const char *path, const char *data, char sync);

extern  CHARGER_INFO_TABLE      ChargerInfo[];
extern  CHARGER_MANAGER         charger_manager;
extern  pthread_rwlock_t        charger_rwlock;

static char *key_init(void);
int do_serv_connect(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_hb(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_charge_req_l(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_charge_req_r(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_stat_update(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_stop_req(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_trl_r(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_trl(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_chaobiao(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_chaobiao_r(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_start_update_r(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_update(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_yuyue_r(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_yuyue(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_config(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_config_r(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_exception(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_start_charge_r(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_start_charge_err(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_stop_charge_r(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_stop_charge(int , BUFF *, CHARGER_INFO_TABLE *);
int do_serv_charge_req(int , BUFF *, CHARGER_INFO_TABLE *);

static char *ev_key = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static ev_proto_l  all_serv [] = {
        {CHARGER_CMD_CONNECT,           do_serv_connect         },
        {CHARGER_CMD_HB,                do_serv_hb              },
        {CHARGER_CMD_CHARGE_REQ,        do_serv_charge_req      },
        {CHARGER_CMD_CHARGE_REQ_R,      do_serv_charge_req_r    },
        {CHARGER_CMD_STATE_UPDATE,      do_serv_stat_update     },
        {CHARGER_CMD_STOP_REQ,          do_serv_stop_req        },
        {CHARGER_CMD_CTRL_R,            do_serv_trl_r           },
        {CHARGER_CMD_CTRL,              do_serv_trl             },
        {CHARGER_CMD_CHAOBIAO,          do_serv_chaobiao        },
        {CHARGER_CMD_CHAOBIAO_R,        do_serv_chaobiao_r      },
        {CHARGER_CMD_START_UPDATE_R,    do_serv_start_update_r  }, // * _R 主动发送
        {CHARGER_CMD_UPDATE,            do_serv_update          },
        {CHARGER_CMD_YUYUE_R,           do_serv_yuyue_r         },
        {CHARGER_CMD_YUYUE,             do_serv_yuyue           },
        {CHARGER_CMD_CONFIG,            do_serv_config          },
        {CHARGER_CMD_CONFIG_R,          do_serv_config_r        },
        {CHARGER_CMD_EXCEPTION,         do_serv_exception       },
        {CHARGER_CMD_START_CHARGE_R,    do_serv_start_charge_r  },
        {CHARGER_CMD_CHARGE_ERROR,      do_serv_start_charge_err},
        {CHARGER_CMD_STOP_CHARGE_R,     do_serv_stop_charge_r   },
        {CHARGER_CMD_STOP_CHARGE,       do_serv_stop_charge     },
        {CHARGER_CMD_NULL,              NULL                    }
};

int 
do_serv_run(int fd, int cmd, BUFF *bf, CHARGER_INFO_TABLE *charger, 
                        void (*err_handler)(int , CHARGER_INFO_TABLE *, BUFF *, int))
{
        ev_proto_l      *proto_arr = all_serv;
        int err;

        while (proto_arr->cmd_type != CHARGER_CMD_NULL)
        {
                if (proto_arr->cmd_type == cmd)
                {
                     err =  proto_arr->cmd_run(fd, bf, charger);
                     break;
                }
                proto_arr++;
        }
        if (err_handler) 
                (*err_handler)(err, charger, bf, bf->ErrorCode);
        return 0; 
}


int do_serv_stop_charge(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        bf->ErrorCode = ESERVER_SEND_SUCCESS;
        return 0;
}

int do_serv_stop_charge_r(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
//        debug_msg("发送停止充电命令, CID[%d] ...", charger->CID);
          debug_msg("Message: cid[%d], send stop charge command: %#x  ", charger->CID, CHARGER_CMD_STOP_CHARGE_R);
        if ( gernal_command(fd, CHARGER_CMD_STOP_CHARGE_R, charger, bf) < 0)
        {
                bf->ErrorCode = EMSG_STOP_CHARGE_ERROR;
                return -1;
        }
//        charger->server_send_status = MSG_STATE_NULL;
//        free(charger->uid);
        bf->ErrorCode = EMSG_STOP_CHARGE_CLEAN;

        return 0;
}

int do_serv_start_charge_err(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
          debug_msg("Message: cid[%d], receive start charge error command: %#x, state: %d ", 
                                        charger->CID, CHARGER_CMD_CHARGE_ERROR, bf->recv_buff[9]);
#if 0
       if (bf->recv_buff[9] == 1)
       {
             debug_msg("充电错误->车未连接, CID[%d] ...", charger->CID);
       } else if (bf->recv_buff[9] == 2)
       {
             debug_msg("充电错误->订单号错误, CID[%d] ...", charger->CID);
       } else if (bf->recv_buff[9] == 3)
       {
            debug_msg("充电错误->UID错误, CID[%d] ...", charger->CID);
       } else if (bf->recv_buff[9] == 4)
       {
            debug_msg("充电错误->套餐号错误, CID[%d] ...", charger->CID);
       }
#endif
       charger->support_max_current = charger->model;
       gernal_command(fd, CHARGER_CMD_HB_R, charger, bf);
       bf->ErrorCode = EMSG_START_CHARGE_ERROR;

       return 0;
}
int do_serv_start_charge_r(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
         debug_msg("Message: cid[%d], send start charge command: %#x ", charger->CID, CHARGER_CMD_START_CHARGE_R);
//       debug_msg("正在发送开始充电命令, CID[%d]", charger->CID);
       if (gernal_command(fd, CHARGER_CMD_START_CHARGE_R, charger, bf) < 0)
       {
             bf->ErrorCode = EMSG_START_CHARGE_ERROR;
             return -1;
       }
       bf->ErrorCode = EMSG_START_CHARGE_CLEAN;
       return 0;
}

int do_serv_exception(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        time_t  tm;
        ev_uint  tmp_4_val, i;
        ev_ushort tmp_2_val;

        debug_msg("Message: cid[%d], receive exception command: %#x  ", charger->CID, CHARGER_CMD_EXCEPTION);
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
        sprintf(bf->val_buff, "[%d-%d-%d %d:%d:%d]", \
                               tim->tm_year+1900, 
                               tim->tm_mon+1, 
                               tim->tm_mday, 
                               tim->tm_hour, 
                               tim->tm_min, 
                               tim->tm_sec);
        free(tim);
        sprintf(bf->val_buff + strlen(bf->val_buff), "CID=%08d:", charger->CID);
        sprintf(bf->val_buff + strlen(bf->val_buff), "presentMode=%d:", bf->recv_buff[13]);
        tmp_4_val = *(unsigned int *)(bf->recv_buff +16);
        sprintf(bf->val_buff + strlen(bf->val_buff), "AccPowr=%d:", tmp_4_val);
        tmp_2_val = *(unsigned short*)(bf->recv_buff + 20);
        sprintf(bf->val_buff + strlen(bf->val_buff), "AChargerFre=%d:", tmp_2_val);
        sprintf(bf->val_buff + strlen(bf->val_buff), "IsConnectDC=%d:", bf->recv_buff[22]);
        tmp_2_val = *(unsigned short*)(bf->recv_buff + 23);
        sprintf(bf->val_buff + strlen(bf->val_buff), "current=%d:", tmp_2_val);
        tmp_2_val = *(unsigned short*)(bf->recv_buff + 25);
        sprintf(bf->val_buff + strlen(bf->val_buff), "voltage=%d:", tmp_2_val);
        sprintf(bf->val_buff + strlen(bf->val_buff), "soc=%d:", bf->recv_buff[27]);
        tmp_2_val = *(unsigned short*)(bf->recv_buff + 28);
        sprintf(bf->val_buff + strlen(bf->val_buff), "tilltime=%d:", tmp_2_val);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 30);
        sprintf(bf->val_buff + strlen(bf->val_buff), "power=%d:", tmp_4_val);
        tmp_2_val = *(unsigned short *)(bf->recv_buff + 34);
        sprintf(bf->val_buff + strlen(bf->val_buff), "usetime=%d:", tmp_2_val);
        bf->send_buff[0] = '\0';
        for (i = 0; i < 16; i++)
        {
              sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[36 + i]);
        }
        sprintf(bf->val_buff + strlen(bf->val_buff), "evlinkid=%s:", bf->send_buff);
        sprintf(bf->val_buff + strlen(bf->val_buff), "chargerway=%d:", bf->recv_buff[52]);
        tmp_2_val = *(unsigned short *)(bf->recv_buff + 53);
        sprintf(bf->val_buff + strlen(bf->val_buff), "chargercode=%d:", tmp_2_val);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 55);
        sprintf(bf->val_buff + strlen(bf->val_buff), "errcode1=%d:", tmp_4_val);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 59);
        sprintf(bf->val_buff + strlen(bf->val_buff), "errcode2=%d:", tmp_4_val);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 63);
        sprintf(bf->val_buff + strlen(bf->val_buff), "errcode3=%d:", tmp_4_val);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 67);
        sprintf(bf->val_buff + strlen(bf->val_buff), "errcode4=%d:", tmp_4_val);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 71);
        sprintf(bf->val_buff + strlen(bf->val_buff), "errcode5=%d:", tmp_4_val);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 75);
        sprintf(bf->val_buff + strlen(bf->val_buff), "yaoxin=%d", tmp_4_val);
        bf->val_buff[strlen(bf->val_buff)] = '\n';
        sprintf(bf->send_buff, "%s/%s/%08d%c", WORK_DIR, EXCEPTION_DIR, charger->CID, '\0');
        write_file(bf->send_buff, bf->val_buff, 0);
       if ( gernal_command(fd, CHARGER_CMD_HB_R, charger, bf) < 0)
       {
              bf->ErrorCode = ESERVER_API_ERR;
              return -1;
       }
       bf->ErrorCode = ESERVER_SEND_SUCCESS;

       return 0;
}

int do_serv_config_r(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        debug_msg("Message: cid[%d], send config command: %#x  ", charger->CID, CHARGER_CMD_CONFIG_R);
        charger->config_num = 0;
        if (gernal_command(fd, CHARGER_CMD_CONFIG_R, charger, bf) < 0)
        {
                bf->ErrorCode = EMSG_CONFIG_ERROR;
               return -1;
        }
        bf->ErrorCode = ESERVER_SEND_SUCCESS;

         return 0;
}

int do_serv_config(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        charger->config_num = bf->recv_buff[9] + 1;
        // 推送完成
        if (bf->recv_buff[10] == 1)
        {
               debug_msg("Message: cid[%d], receive config command: %#x, state: finish ", charger->CID, CHARGER_CMD_CONFIG);
               bf->ErrorCode = MSG_CONFIG_FINISH;
               return 0;
        }
        if (gernal_command(fd, CHARGER_CMD_CONFIG_R, charger, bf) < 0)
        {
               bf->ErrorCode = EMSG_CONFIG_ERROR;
               return -1;
        }
        bf->ErrorCode = ESERVER_SEND_SUCCESS;

        return 0;
}

int do_serv_yuyue(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        debug_msg("Message: cid[%d], receive yuyue command: %#x, state: finish ", charger->CID, CHARGER_CMD_YUYUE);
        bf->ErrorCode = MSG_YUYUE_FINISH;
        return 0;
}

int do_serv_yuyue_r(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        debug_msg("Message: cid[%d], send yuyue command: %#x ", charger->CID, CHARGER_CMD_YUYUE_R);
        if (gernal_command(fd, CHARGER_CMD_YUYUE_R, charger, bf) < 0)
         {
                  bf->ErrorCode = EMSG_YUYUE_ERROR;
                  return -1;
        }
         bf->ErrorCode = ESERVER_SEND_SUCCESS;
         return 0;
}
int do_serv_update(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
	if(bf->recv_buff[12] == 1)
        {            // 线程不安全，未作处理

               debug_msg("Message: cid[%d], receive update command: %#x, package: %d, progress: %d%%", 
                                       charger->CID, CHARGER_CMD_UPDATE, bf->recv_buff[11], 100);
               bf->ErrorCode = MSG_UPDATE_FINISH; 
               return 0;
        } else
        {
	        if (bf->recv_buff[11] % 10 == 0)
                debug_msg("Message: cid[%d], receive update command: %#x, package: %d, progress: %d%%", 
                                       charger->CID, CHARGER_CMD_UPDATE, bf->recv_buff[11], 
                                      ( (float)(bf->recv_buff[11])/(float)(charger->file_length & 1023) * 100.0));
            
        }
        if ( gernal_command(fd, CHARGER_CMD_UPDATE_R, charger, bf) < 0)
        {
               bf->ErrorCode = EMSG_UPDATE_ERROR;
               return -1;
        }
        bf->ErrorCode = ESERVER_SEND_SUCCESS;

        return 0;;
}

int do_serv_start_update_r(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        debug_msg("Message: cid[%d], send start update command: %#x ", charger->CID, CHARGER_CMD_START_UPDATE_R);
       if (gernal_command(fd, CHARGER_CMD_START_UPDATE_R, charger, bf) < 0) 
       {
             bf->ErrorCode = EMSG_UPDATE_ERROR;
             return -1;
       }
       bf->ErrorCode = ESERVER_SEND_SUCCESS;
 
       return 0;
}

int do_serv_chaobiao_r(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        printf("Message: cid[%d], send start chaobiao command: %#x ", charger->CID, CHARGER_CMD_CHAOBIAO_R);
        charger->cb_target_id  = 0;
        charger->cb_charging_code  = 0;
        if (gernal_command(fd, CHARGER_CMD_CHAOBIAO_R, charger, bf) < 0)
        {
               bf->ErrorCode = EMSG_CHAOBIAO_ERROR;
               return -1;
        }
        bf->ErrorCode = ESERVER_SEND_SUCCESS;
        return 0;
}
int do_serv_chaobiao(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
//        printf("接收到抄表记录:CID[%d], total = %d, present_cnt:%d, complete:%d", \
//                                         charger->CID, 
//                                         *(unsigned short *)(bf->recv_buff + 984), 
//                                         bf->recv_buff[981], 
//                                         bf->recv_buff[986]);

        if (bf->recv_buff[986] == 0)
        {
                charger->cb_charging_code = (*(unsigned short *)(bf->recv_buff + 982)) - 15;
                charger->cb_target_id = 0;
        }else
        {
                charger->cb_charging_code =  0;
                charger->cb_target_id = 0xFFFF;
        }
        //写充电记录到文件
         if(bf->recv_buff[981] > 0 && write(charger->file_fd, bf->recv_buff + 21, 64*bf->recv_buff[981]) != 64*bf->recv_buff[981])
         {
               debug_msg("write chaobiao failed");
               bf->ErrorCode = EMSG_CHAOBIAO_ERROR;
               return -1;
         }
         if (gernal_command(fd, CHARGER_CMD_CHAOBIAO_R, charger, bf) < 0)
         {
                 if (bf->recv_buff[986] != 1)
                         bf->ErrorCode = EMSG_CHAOBIAO_ERROR;
                 else 
                  {
                        bf->ErrorCode = MSG_CHAOBIAO_FINISH;
                        return 0;
                  }
                  return -1;
         }
         if (bf->recv_buff[986] == 1)
         {
                 debug_msg("Message: cid[%d], receive chaobiao command: %#x, record: %d, state: finish ", 
                          charger->CID, CHARGER_CMD_CHAOBIAO_R, *(unsigned short *)(bf->recv_buff + 984));
                 bf->ErrorCode = MSG_CHAOBIAO_FINISH;
         }
         else
                 bf->ErrorCode = ESERVER_SEND_SUCCESS;
        
        return 0;
}

int do_serv_trl(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        debug_msg("Message: cid[%d], receive control command: %#x, state: finish ", charger->CID, CHARGER_CMD_CTRL);
        if (bf->recv_buff[13] == 1)
        {
                bf->ErrorCode = MSG_CONTROL_FINISH;
                return 0;
        }
        bf->ErrorCode = MSG_CONTROL_NO_CTRL;
        return 0;        
}
int do_serv_trl_r(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        debug_msg("Message: cid[%d], send control command: %#x  ", charger->CID, CHARGER_CMD_CTRL_R);
        if (gernal_command(fd, CHARGER_CMD_CTRL_R, charger, bf) < 0)
        {
                bf->ErrorCode = ESERVER_API_ERR;
                return -1;
        }
        bf->ErrorCode = ESERVER_SEND_SUCCESS;
        
        return 0;
}

int do_serv_stop_req(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        ev_ushort  tmp_2_val, i;
        ev_uint    tmp_4_val;
        struct finish_task task;
        time_t     end_time;
        
        printf("接受到充电结束...\n");
	end_time = (bf->recv_buff[13] << 24 | bf->recv_buff[14] << 16 | bf->recv_buff[15] << 8 | bf->recv_buff[16]);				//获取充电结束时间
	charger->is_charging_flag = 0;
        if (bf->recv_buff[59] == 1) // 扫码
                goto replay58;       
	// 发送数据给后台
	sprintf(bf->val_buff, "/ChargerState/stopState?");
	sprintf(bf->val_buff + strlen(bf->val_buff), "key={chargers:[{chargerId:\\\"%08d\\\",", charger->CID);
        bf->send_buff[0] = '\0';
        for (i = 0; i < 16; i++) {
            sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[43 + i]);
        }
        sprintf(bf->val_buff + strlen(bf->val_buff), "privateID:\\\"%s\\\",", bf->send_buff);
	tmp_4_val = *(unsigned int *)(bf->recv_buff + 37);
	sprintf(bf->val_buff + strlen(bf->val_buff), "power:%d,", tmp_4_val);
	tmp_2_val =(bf->recv_buff[60] << 8 | bf->recv_buff[61]);
	sprintf(bf->val_buff + strlen(bf->val_buff), "chargingRecord:%d,", tmp_2_val);
	sprintf(bf->val_buff + strlen(bf->val_buff), "mac:\\\"%s\\\",", charger->MAC);
	sprintf(bf->val_buff + strlen(bf->val_buff), "chargingType:%d,", bf->recv_buff[59]);
	sprintf(bf->val_buff + strlen(bf->val_buff), "status:%d}]}",  CHARGER_CHARGING_COMPLETE_LOCK);
        if (opt_type & OPTION_LONG)
        {
                if( (task.str = (char *)malloc(strlen(bf->val_buff) + 1) ) != NULL)
                {
                        task.cmd = WAIT_CMD_STOP_STATE_API;
                        task.cid = charger->CID;
                        task.strlen = strlen(bf->val_buff);
                        strcpy(task.str, bf->val_buff);
                        finish_task_add(SERVER_WAY, &task);
                }
        } else
        {
//                cmd_frun("dashboard url_post %s %s", sys_check_url(), bf->val_buff);
        }
replay58:            
	//写入数据库，更新相应表信息
	//当前模式
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 22);
        sprintf(bf->val_buff, "%d", tmp_4_val);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.AccPower", charger->tab_name);
        uci_clean_charge_finish(charger);
        tmp_2_val = (bf->recv_buff[41] << 8 | bf->recv_buff[42]);
        sprintf(bf->val_buff, "cid:%08d,start_tm:%d,end_tm:%d,pow:%d,way:%d,cycid:%d, uid:", 
                                        charger->CID, 
                                        end_time - tmp_2_val, 
                                        end_time, 
                                        charger->power,
                                        bf->recv_buff[59],
                                        (bf->recv_buff[60] << 8 | bf->recv_buff[61]));
        if (bf->recv_buff[59] != 1) // 不是扫码,不追加uid
        {
                sprintf(bf->val_buff + strlen(bf->val_buff), "%s", bf->send_buff);
        }
        bf->val_buff[strlen(bf->val_buff)] = '\n';
        bf->val_buff[strlen(bf->val_buff) + 1] = '\0';
        sprintf(bf->send_buff, "%s/%s/%08d", WORK_DIR, RECORD_DIR, charger->CID);
        write_file(bf->send_buff, bf->val_buff, EV_FILE_SYNC);
        bf->val_buff[strlen(bf->val_buff)] = 0;
        debug_msg("Message: cid[%d], receive stop-charge-req command: %#x  \n  ----info: %s ", 
                charger->CID, CHARGER_CMD_STOP_REQ, bf->val_buff);
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
}

int do_serv_stat_update(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        ev_ushort tmp_2_val, i;
        ev_uint   tmp_4_val;
        struct  finish_task task;
        time_t   tm;

        charger->present_status = MSG_CHARGER_CHARGING;
        printf("接受到状态更新\n");
        charger->is_charging_flag = 1;
        charger->real_time_current = bf->recv_buff[22];
	sprintf(bf->val_buff, "%d", bf->recv_buff[52]);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerWay", charger->tab_name);
        sprintf(bf->val_buff, "%d", bf->recv_buff[17]);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentMode", charger->tab_name);
	tmp_2_val = (bf->recv_buff[23] << 8 | bf->recv_buff[24]);
        sprintf(bf->val_buff, "%d", tmp_2_val);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentOutputCurrent", charger->tab_name);
	tmp_2_val = (bf->recv_buff[25] << 8 | bf->recv_buff[26]);
        sprintf(bf->val_buff, "%d", tmp_2_val);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentOutputVoltage", charger->tab_name);
	tmp_2_val = *(unsigned short *)(bf->recv_buff+34);
	sprintf(bf->val_buff, "%d", tmp_2_val);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Duration", charger->tab_name);
        tmp_2_val = *(unsigned short *)(bf->recv_buff + 53);
        sprintf(bf->val_buff, "%d", tmp_2_val);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargingCode", charger->tab_name);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 30);
	sprintf(bf->val_buff, "%d", tmp_4_val);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Power", charger->tab_name);
        if (bf->recv_buff[52] != 1) // 拍卡
        {
              bf->val_buff[0] = '\0';
              for (i = 0; i < 16; i++)
                       sprintf(bf->val_buff + strlen(bf->val_buff), "%02x", bf->recv_buff[36 + i]);
	        ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.privateID", charger->tab_name);
         }
         // 快冲
        if (charger->charger_type == 2 || charger->charger_type == 4)
        {
                if (opt_type & OPTION_LONG)
                {
                        sprintf(bf->val_buff, "%d", bf->recv_buff[27]);
                        ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Soc", charger->tab_name);
                        tmp_2_val = (bf->recv_buff[28] << 8 | bf->recv_buff[29]);
                        sprintf(bf->val_buff, "%d", tmp_2_val);
	                ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Tilltime", charger->tab_name);
                        task.cmd = WAIT_CMD_UPDATE_FAST_API;
                        task.cid = charger->CID; 
                        finish_task_add(SERVER_WAY, &task);
                } else
                    cmd_frun("dashboard update_fast");
         }
          // 记录日志,电量
          struct tm *tim1 = (struct tm *)malloc(sizeof(struct tm));
          if (tim1 != NULL)
          {
                tm = *(time_t *)(bf->recv_buff + 13);
                if (localtime_r(&tm, tim1) != NULL)
                {
                       sprintf(bf->send_buff, "%s/%s/%08d", WORK_DIR, LOG_DIR, charger->CID);
                       sprintf(bf->val_buff, "[%4d-%02d-%02d %02d:%02d:%02d]", 
                                                tim1->tm_year+1900, 
                                                tim1->tm_mon+1, 
                                                tim1->tm_mday, 
                                                tim1->tm_hour, 
                                                tim1->tm_min, 
                                                tim1->tm_sec);
                        sprintf(bf->val_buff + strlen(bf->val_buff),
                                             "CID:%08d, cycid:%4d, cur:%2dA, vol:%3dV,power:%dwh, soc:%d, tilltime:%d\n",
                                                charger->CID, 
                                                ((bf->recv_buff[53] << 8)|bf->recv_buff[54]),
                                                (bf->recv_buff[23]<<8 | bf->recv_buff[24]),
                                                (bf->recv_buff[25]<<8 | bf->recv_buff[26]), 
                                                *(unsigned int *)( bf->recv_buff + 30), 
                                                bf->recv_buff[27], 
                                                (bf->recv_buff[28] << 8| bf->recv_buff[29]));
                       write_file(bf->send_buff, bf->val_buff, 0);
                }
                free(tim1);
           }
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
}

int do_serv_charge_req_r(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
           printf("system_message:%d, target_mode:%d\n", charger->system_message, charger->target_mode);
           if (gernal_command(fd, CHARGER_CMD_CHARGE_REQ_R, charger, bf) < 0)
           {
                  bf->ErrorCode = ESERVER_API_ERR;
                 return -1;
           }
           bf->ErrorCode = ESERVER_SEND_SUCCESS;
         
           return 0;
}

int do_serv_charge_req(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
         // 数据加入发送链表
        int i;
        struct finish_task task;
        
        sprintf(bf->val_buff, "%s", "/Charging/canStartCharging?");
        sprintf(bf->val_buff +strlen(bf->val_buff), "chargerID=%08d'&'", charger->CID);
        bf->send_buff[0] = 0;
        for (i = 0; i < 16; i++)
               sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[36 + i]);
        sprintf(bf->val_buff + strlen(bf->val_buff), "privateID=%s", bf->send_buff);
        if (opt_type & OPTION_LONG)
        {
                do_serv_charge_req_l(fd, bf, charger);
                if (bf->recv_buff[56] != 1) // 拍卡
                {
                        if ( (task.str = (char *)malloc(strlen(bf->val_buff) + 1)) == NULL)
                                return ESERVER_API_ERR;
                        strcpy(task.str, bf->val_buff);
                        task.cid = charger->CID;
                        task.strlen = strlen(bf->val_buff);
                        task.cmd = WAIT_CMD_CHARGER_API;
                        finish_task_add(SERVER_WAY, &task);
                        //bf->ErrorCode = ESERVER_SEND_SUCCESS;
                        return 0;
                }
        } else
        {
               // load balance
               do {
                        if (bf->recv_buff[56] == 1)
                              break;
                        char *ptr = check_send_status(bf->val_buff);
                        if ( !ptr)
                        {
                                if (charger->charger_type == 2 || charger->charger_type == 4)
                                 {
                                        charger->system_message = SYM_OFF_NET;
                                        charger->target_mode = CHARGER_READY;
                                } else
                                {
                                        charger->system_message = 0x01;
                                        charger->target_mode = CHARGER_CHARGING;
                                }
                        } else
                        {
                                charger->system_message = ptr[8] - 48;
                                charger->target_mode = (charger->system_message == 0x01) ? CHARGER_CHARGING : CHARGER_READY;
//                                free(ptr);
                        }
               } while(0);
                do_serv_charge_req_l(fd, bf, charger);
        }
        do_serv_charge_req_r(fd, bf, charger);
        debug_msg("Message: cid[%d], receive charge-req command: %#x, uid: %s, system_message: %d, target_mode: %d\n", 
              charger->CID, CHARGER_CMD_CHARGE_REQ, (bf->recv_buff[56] == 1) ? "null" : bf->send_buff, 
              charger->system_message, charger->target_mode);
        bf->ErrorCode = ESERVER_SEND_SUCCESS;
        return 0; 

}

int do_serv_charge_req_l(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        int i;
        struct finish_task task;
        ev_ushort  tmp_2_val;
        ev_uint    tmp_4_val;
        
       if (bf->recv_buff[56] == 1)
        {
                charger->system_message = 0x01;
                charger->target_mode = CHARGER_CHARGING;
//                charger->start_time = time(0);	//开始充电时间
	        charger->charging_code = *(unsigned short *)(bf->recv_buff+34);			//赋值charging_code
                // 扫码充电
                debug_msg("扫码允许充电 ...\n");
                goto reply_to_charger1;
         }
#if 0
         // 数据加入发送链表
        sprintf(bf->val_buff, "%s", "/Charging/canStartCharging?");
        sprintf(bf->val_buff +strlen(bf->val_buff), "chargerID=%08d'&'", charger->CID);
        bf->send_buff[0] = '\0';
        for (i = 0; i < 16; i++)
               sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[36 + i]);
        sprintf(bf->val_buff + strlen(bf->val_buff), "privateID=%s", bf->send_buff);
        memcpy(charger->ev_linkid, bf->recv_buff+36, 16);
        if ( (task.str = (char *)malloc(strlen(bf->val_buff) + 1)) == NULL)
              return ESERVER_API_ERR;
        strcpy(task.str, bf->val_buff);
        task.cid = charger->CID;
        task.strlen = strlen(bf->val_buff);
        task.cmd = WAIT_CMD_CHARGER_API;
        finish_task_add(SERVER_WAY, &task);
#endif
        // 写入数据
           sprintf(bf->val_buff, "%s/%s/%08d_startlog", WORK_DIR, LOG_DIR, charger->CID);
           char *data_req = bf->val_buff + strlen(bf->val_buff) + 10;
           data_req[0] = '\0';
           snprintf(data_req, (MAX_LEN - strlen(bf->val_buff) -11), 
                                "cid:%08d,uid:%s,cycid:%d,tm:%d \n", 
                                charger->CID, 
                                bf->send_buff, 
                                *(unsigned short*)(bf->recv_buff + 34), 
                                *(int *)(bf->recv_buff + 13));
           write_file(bf->val_buff, data_req, EV_FILE_SYNC);
         //写入uci数据库
//			ev_uci_save_action(UCI_SAVE_OPT, true, bf->send_buff, "chargerinfo.%s.privateID", charger->tab_name);
reply_to_charger1:
	sprintf(bf->val_buff, "%d", bf->recv_buff[17]);
        ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentMode", charger->tab_name);
        tmp_2_val = *(unsigned short *)(bf->recv_buff + 18);
//			sprintf(bf->val_buff, "%d", tmp_2_val);
//			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.SubMode", charger->tab_name);
//			sprintf(bf->val_buff, "%d", bf->recv_buff[29]);	//select current
//			ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.SelectCurrent", charger->tab_name);
	sprintf(bf->val_buff, "%d", *(unsigned short *)(bf->recv_buff + 30));	//presentoutputcurrent
        ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentOutputCurrent", charger->tab_name);
	tmp_2_val = *(unsigned short *)(bf->recv_buff+34);
	sprintf(bf->val_buff, "%d", tmp_2_val);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargingCode", charger->tab_name);
	sprintf(bf->val_buff, "%d", bf->recv_buff[56]);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerWay", charger->tab_name);
        tmp_4_val = *(unsigned int *)(bf->recv_buff + 22);
        sprintf(bf->val_buff, "%d", tmp_4_val);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.AccPower", charger->tab_name);
        ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.Duration", charger->tab_name);
	ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.Power", charger->tab_name);
 
        return 0;
}

int do_serv_hb(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        unsigned char status = bf->recv_buff[9];

        printf("接收到心跳...\n");
        charger->is_charging_flag = 0;
        sprintf(bf->val_buff, "%d", bf->recv_buff[9]);
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentMode", charger->tab_name);
        ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.PresentOutputCurrent", charger->tab_name);
	ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.SelectCurrent", charger->tab_name);
        sprintf(bf->val_buff, "%d", ((bf->recv_buff[10] << 8) | bf->recv_buff[11])); // SUB_MODE
	ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.SubMode", charger->tab_name);
      
        printf("digtaloutput:%#x\n", bf->recv_buff[14] << 8 | bf->recv_buff[15]); 
        printf("digtalinput:%#x\n",  bf->recv_buff[16] << 8 | bf->recv_buff[17]); 
        if (status == CHARGER_READY)
                charger->present_status = MSG_CHARGER_READY;
        else if (status == CHARGER_CHARGING_COMPLETE_LOCK || status == CHARGER_CHARGING)
               charger->present_status = MSG_CHARGER_CHARGING;
        else  
              charger->present_status = MSG_PERM_REFUSED;

        if (gernal_command(fd, CHARGER_CMD_HB_R, charger, bf) < 0){
                bf->ErrorCode = ESERVER_API_ERR;
                return -1;
        }
        bf->ErrorCode = ESERVER_SEND_SUCCESS;
        return 0;
}

int do_serv_connect(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger)
{
        char    mac_addr[50], *key, *sptr = NULL, offset;
        int i;
        sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", 
                           bf->recv_buff[42],
                           bf->recv_buff[43],
                           bf->recv_buff[44],
                           bf->recv_buff[45],
                           bf->recv_buff[46],
                           bf->recv_buff[47]);
         if ( !(key = key_init()) ) {
             // 写日志
             bf->ErrorCode = ESERVER_API_ERR;
             goto error;
         }
        
         // 判断电桩信息是否存在
         if  (charger )
         {
		//将改变的IP存入数据库
		if(ev_uci_save_action(UCI_SAVE_OPT, true, mac_addr, "chargerinfo.%s.MAC", charger->tab_name )  < 0)
                       goto error;
		sprintf(bf->val_buff, "%d.%d.%d.%d", bf->recv_buff[9], bf->recv_buff[10], bf->recv_buff[11], bf->recv_buff[12]);
		if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.IP", charger->tab_name) < 0) 
                       goto error;
                strcpy(bf->val_buff, key);
	        if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.KEYB", charger->tab_name) < 0)
                      goto error;
                bf->val_buff[11] = 0;
                memcpy(bf->val_buff, bf->recv_buff + 18, 10);
	        if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Model", charger->tab_name) < 0)
                      goto error;
                memcpy(bf->val_buff, bf->recv_buff + 28, 10);
	        if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Series", charger->tab_name) < 0)
                      goto error;
                sprintf(bf->val_buff, "%d", bf->recv_buff[48]);
	        if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerType", charger->tab_name) < 0)
                      goto error;
                sprintf(bf->val_buff, "%d.%02d", bf->recv_buff[41], bf->recv_buff[40]);
	        if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerVersion", charger->tab_name) < 0)
                      goto error;
                memcpy(bf->val_buff, bf->recv_buff + 49, 12);
                bf->val_buff[12] = 0;
                if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.FwName", charger->tab_name) < 1)	
			goto error;
                
                memcpy(charger->KEYB, key, 16);
                charger->charger_type = bf->recv_buff[48];
                memcpy(bf->val_buff, bf->recv_buff+18, 10);
                if (!strncmp(bf->val_buff, "EVG-16N", 7))
                      charger->model = EVG_16N;
                else if (!strncmp(bf->val_buff, "EVG-32N",7))
                      charger->model = EVG_32N;
                else if (!strncmp(bf->val_buff, "EVG-32NW", 8))
                      charger->model = EVG_32N;
                else if (!strncmp(bf->val_buff, "EVG-32N", 7))
                      charger->model = EVG_32N;
         } else
         {
                printf("aa\n");
                char   same_charger = 0, offset, tab_buff[50];
                
                if( pthread_rwlock_wrlock(&charger_rwlock) < 0){
                       exit(EXIT_FAILURE);
	         }
		if( (sptr = find_uci_tables(TAB_POS)) )
	        {
                       for (i = 0; i < charger_manager.present_charger_cnt; i++)
                        {
                             if( memcmp(ChargerInfo[i].MAC, mac_addr, strlen(mac_addr)) == 0)
                              {
                                 offset = i;
                                 same_charger = 1;
                                 sprintf(bf->val_buff, "%08d", *(unsigned int *)(bf->recv_buff+5));
	                        if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.CID", ChargerInfo[i].tab_name) < 0)
		                        goto error;
                            }
                     }
		} 
	        if (same_charger == 1)
                {
                      strcpy(tab_buff, ChargerInfo[offset].tab_name);
                }
                else
                {
                      offset = charger_manager.present_charger_cnt;
	              sprintf(tab_buff, "charger%d", charger_manager.present_charger_cnt+1);
	              ev_uci_add_named_sec("chargerinfo.%s=1", tab_buff);//创建changer表
                        if (sptr)
                        {
                                sprintf(sptr + strlen(sptr), ",%s", tab_buff);
		                ev_uci_save_action(UCI_SAVE_OPT, true, sptr, "chargerinfo.TABS.tables");
                        }
                         else
		                ev_uci_save_action(UCI_SAVE_OPT, true, tab_buff, "chargerinfo.TABS.tables");
                 }

		// 保存CID，IP， KEYD到数据库
		if(ev_uci_save_action(UCI_SAVE_OPT, true, mac_addr, "chargerinfo.%s.MAC", tab_buff )  < 1)
	                goto error;
		sprintf(bf->val_buff, "%08d", *(unsigned int *)(bf->recv_buff + 5));
		if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.CID", tab_buff )  < 1)
			goto error;
		sprintf(bf->val_buff, "%d.%d.%d.%d", bf->recv_buff[9], bf->recv_buff[10], bf->recv_buff[11], bf->recv_buff[12]);	// IP
		if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.IP", tab_buff )  < 1)
			goto error;
		strcpy(bf->val_buff, key);
		if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.KEYB", tab_buff)  < 1)
			goto error;
                bf->recv_buff[11] = 0;
		memcpy(bf->val_buff, bf->recv_buff+18, 10);
		if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Model", tab_buff) < 1) // series
			goto error;
		memcpy(bf->val_buff, bf->recv_buff+28, 10);
		if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Series", tab_buff) < 1)	
			goto error;
		sprintf(bf->val_buff, "%d.%02d", bf->recv_buff[41], bf->recv_buff[40]);	//chargerVersion
		if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerVersion", tab_buff) < 1)	
			goto error;
                sprintf(bf->val_buff, "%d", bf->recv_buff[48]);
		if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerType", tab_buff) < 1)	
			goto error;
		memcpy(bf->val_buff, bf->recv_buff + 49, 12);
                bf->val_buff[12] = 0;
                if(ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.FwName", tab_buff) < 1)	
			goto error;
                sprintf(bf->val_buff, "%d", bf->recv_buff[17]);
	        ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.PresentMode", tab_buff);	
                ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.SelectCurrent", tab_buff);	
		ev_uci_save_action(UCI_SAVE_OPT, true, "0", "chargerinfo.%s.PresentOutputCurrent", tab_buff);
		// 写入数据库
		ChargerInfo[offset].CID = *(unsigned int *)(bf->recv_buff + 5);
                printf("cid:%d, offset:%d\n", ChargerInfo[offset].CID, offset);
                ChargerInfo[offset].charger_type = bf->recv_buff[48];
                ChargerInfo[offset].wait_cmd = WAIT_CMD_NONE;
                strcpy(ChargerInfo[offset].MAC, mac_addr);
		memcpy(ChargerInfo[offset].KEYB, key, 16);
		strcpy(ChargerInfo[offset].tab_name, tab_buff);
		memcpy(bf->val_buff, bf->recv_buff+18, 10);
                charger_manager.present_charger_cnt++;
		if(!strncmp(bf->val_buff, "EVG-16N", 7))
			ChargerInfo[offset].model = EVG_16N;
		else if(!strncmp(bf->val_buff, "EVG-32N", 7))
			ChargerInfo[offset].model = EVG_32N;	
		else if(!strncmp(bf->val_buff, "EVG-32NW", 8))
                        ChargerInfo[offset].model = EVG_32N;
                if (pthread_rwlock_unlock(&charger_rwlock) < 0)
                        exit(EXIT_FAILURE); 
                if (gernal_command(fd, CHARGER_CMD_CONNECT_R, &ChargerInfo[offset], bf) < 0)
                     goto error;
                bf->ErrorCode = ESERVER_SEND_SUCCESS;
                return offset;
         }
         if (gernal_command(fd, CHARGER_CMD_CONNECT_R, &ChargerInfo[offset], bf) < 0)
                goto error;
         bf->ErrorCode = ESERVER_SEND_SUCCESS;
         return 0; 
error:
        if (key != NULL)
                free(key);
        if (sptr != NULL)
                free(sptr);
        bf->ErrorCode = ESERVER_API_ERR;
        return -1;
}
static char *key_init(void)
{
        char *val, i;
        struct timeval  tpstart;

        if ( !(val = (char *)malloc(17)) )
            return NULL;
         for(i=0; i<16; i++)
         {
                 if(gettimeofday(&tpstart, NULL)<0){      
                     free(val);
                     return NULL;
                 }
                 srand(tpstart.tv_usec);
                 *(val+i)= *(ev_key + rand()%strlen(ev_key) );
                 msleep(5);
         }
         val[16] = 0;

         return val;
}
