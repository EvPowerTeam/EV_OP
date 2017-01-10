

#include "include/protocal_charger.h"
#include "include/AES.h"
#include "include/CRC.h"
#include "include/err.h"
#include "include/ev_command_file.h"

extern  CHARGER_MANAGER charger_manager;
extern  int opt_type;
static void protocal_init_head(unsigned char CMD,  char *p_str, unsigned int cid);

void SendServer_charger_status(CHARGER_INFO_TABLE *charger, BUFF *bf)
{
        
        unsigned char  i, new_mode = 0, cmd = bf->recv_buff[4];
        struct finish_task  task;

        if ( cmd == CHARGER_CMD_HB )
            new_mode = bf->recv_buff[9];
        else if ( cmd == CHARGER_CMD_CHARGE_REQ || cmd == CHARGER_CMD_STATE_UPDATE || cmd == CHARGER_CMD_STOP_REQ)
            new_mode = bf->recv_buff[17];
            
//        printf("recv_cmd:%#x, cmd:%#x, present_mode:%d, new_mode:%d\n", bf->recv_buff[4], cmd, charger->present_mode, new_mode);

        if ( new_mode ==0 ||  new_mode == charger->present_mode )
            return ;
       
        if ( charger->present_mode > 0)
        {
            if (new_mode == CHARGER_CHARGING && bf->recv_buff[4] == CHARGER_CMD_STATE_UPDATE)
            {
                sprintf(bf->val_buff, "/ChargerState/stopState?");
                sprintf(bf->val_buff + strlen(bf->val_buff), "key={chargers:[{chargerId:\\\"%08d\\\",", charger->CID);
                bf->send_buff[0] = 0;
                for (i = 0; i < 16; i++)
                {
                        sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[36 + i]);
                }
                sprintf(bf->val_buff + strlen(bf->val_buff), "privateID:\\\"%s\\\",", bf->send_buff);
                sprintf(bf->val_buff + strlen(bf->val_buff), "power:%d,", ((bf->recv_buff[30]<<24)|(bf->recv_buff[31]<<16)|(bf->recv_buff[32]<<8)|bf->recv_buff[33]));
                sprintf(bf->val_buff + strlen(bf->val_buff), "chargingRecord:%d,", (bf->recv_buff[53] << 8 | bf->recv_buff[54]));
                sprintf(bf->val_buff + strlen(bf->val_buff), "mac:\\\"%s\\\",", charger->MAC);
                sprintf(bf->val_buff + strlen(bf->val_buff), "chargingType:%d,", bf->recv_buff[52]);
                sprintf(bf->val_buff + strlen(bf->val_buff), "status:%d}]}", CHARGER_CHARGING);
                printf("send:%s\n", bf->val_buff);
                if ( (task.str = (char *)malloc(strlen(bf->val_buff) +1)) != NULL)
                 {
                      task.cid = charger->CID;
                      task.cmd = WAIT_CMD_STOP_STATE_API;
                      strcpy(task.str, bf->val_buff);
                      finish_task_add(SERVER_WAY, &task);
                 }
            }
            if (new_mode == CHARGER_READY)
                   uci_clean_charge_finish(charger);


            sprintf(bf->val_buff, "/ChargerState/changesStatus?key={cid:\\\"%08d\\\",status:%d}", charger->CID, new_mode);
            if ( (task.str = (char *)malloc(strlen(bf->val_buff) + 1)) != NULL)
             {
                     task.cid = charger->CID;
                     task.cmd = WAIT_CMD_CHANGE_STATUS_API;
                     strcpy(task.str, bf->val_buff);
                     finish_task_add(SERVER_WAY, &task);
             }
            debug_msg("Message: cid[%d], PresentMode changes:from %d to %d  ", charger->CID, charger->present_mode, new_mode);
        }
        charger->present_mode = new_mode;
        return ; 
}

void 
uci_clean_charge_finish(const CHARGER_INFO_TABLE *charger)
{

            ev_uci_delete( "chargerinfo.%s.privateID", charger->tab_name);
			ev_uci_delete( "chargerinfo.%s.ChargingCode", charger->tab_name);
			ev_uci_delete( "chargerinfo.%s.PresentOutputVoltage", charger->tab_name);
			ev_uci_delete( "chargerinfo.%s.ChargerWay", charger->tab_name);
			ev_uci_delete( "chargerinfo.%s.Power", charger->tab_name);
			ev_uci_delete( "chargerinfo.%s.Duration", charger->tab_name);
    
            if (charger->charger_type == 2 || charger->charger_type == 4)
            {
			    ev_uci_delete( "chargerinfo.%s.Soc", charger->tab_name);
			    ev_uci_delete( "chargerinfo.%s.Tilltime", charger->tab_name);
            }
}
void 
error_handler(int handle, CHARGER_INFO_TABLE *charger, BUFF *bf, int errnum)
{
    struct finish_task    task;
    int     i;
    // 操作成功
    if (bf->recv_buff[4] == 0x10 || (!charger) )
        return ;

    task.cmd = MSG_CMD_FROM_CHARGER;
    task.way = charger->way;
    
    if ( !handle)
    {
//        printf("本次通信成功:CID[%d] ...\n", *(unsigned int *)(bf->recv_buff + 5));
        if (errnum == ESERVER_SEND_SUCCESS)
         { 
                SendServer_charger_status(charger, bf);
                return;
         }
        
        if (errnum == MSG_CHAOBIAO_FINISH)     //抄表完成
        {
//             task.cmd = charger->wait_cmd;
             task.cid = charger->CID;
             task.errcode = MSG_CHAOBIAO_FINISH;
             task.u.chaobiao.start_time = charger->cb_start_time;
             task.u.chaobiao.end_time = charger->cb_end_time;
             task.u.chaobiao.chargercode = *(unsigned short *)(bf->recv_buff +984);
             charger->wait_cmd = WAIT_CMD_NONE;
             finish_task_add(charger->way, &task);
        
        } else if (errnum == MSG_UPDATE_FINISH) // 更新软件完成
        {
//             task.cmd = charger->wait_cmd;
             task.cid = charger->CID;
             task.errcode = MSG_UPDATE_FINISH;
             charger->wait_cmd = WAIT_CMD_NONE;
             finish_task_add(charger->way, &task);
        
        }else if (MSG_CONFIG_FINISH == errnum) // 推送配置完成
        {
//               task.cmd = charger->wait_cmd;
               task.cid = charger->CID;
               task.errcode = MSG_CHAOBIAO_FINISH;
               close(charger->file_fd); 
               charger->wait_cmd = WAIT_CMD_NONE;
               finish_task_add(charger->way, &task);
        } else if (MSG_YUYUE_FINISH == errnum)
        {
               task.cid = charger->CID;
               task.cmd = WAIT_CMD_PUSH_MESSAGE_API;
               bf->send_buff[0] = 0;
               for (i = 0; i < 16; i++)
               {
                    sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[9 + i]);
               }
               sprintf(bf->val_buff, "/PushMessage/pushMessage?cid=%08d\\\&type=%d\\\&code=%d\\\&id=%s",
                       charger->CID, 3, 301, bf->send_buff);
               if ( (task.str = (char *)malloc(strlen(bf->val_buff) + 1)) != NULL)
               {
                       strcpy(task.str, bf->val_buff);
                       finish_task_add(SERVER_WAY, &task);
               }

               task.cmd = MSG_CMD_FROM_CHARGER;
               task.errcode = MSG_YUYUE_FINISH;
               sprintf(bf->val_buff, "/reserve?key={cid:\\\"%08d\\\",pid:\\\"%s\\\",status:%d}",
                       charger->CID, bf->send_buff, CHARGER_RESERVED);
               if ( (task.str = (char *)malloc(strlen(bf->val_buff) + 1)) != NULL)
               {
                       strcpy(task.str, bf->val_buff);
                       finish_task_add(SERVER_WAY, &task);
               }
        } else if (MSG_CONTROL_FINISH == errnum)
        {
                task.errcode = MSG_CONTROL_FINISH;
                task.cid = charger->CID;
                task.u.control.status = 1;
                charger->wait_cmd = WAIT_CMD_NONE;
                finish_task_add(charger->way, &task);    
        } else if (MSG_CONTROL_NO_CTRL == errnum)
        {
                task.cid = charger->CID;
                task.u.control.status = 0;
                charger->wait_cmd = WAIT_CMD_NONE;
                finish_task_add(charger->way, &task);    
        } else if (MSG_START_CHARGE_FINISH == errnum)
        {
//                task.cmd = charger->wait_cmd;
                task.cid = charger->CID;
                task.errcode = MSG_START_CHARGE_FINISH;
                charger->wait_cmd = WAIT_CMD_NONE;
                finish_task_add(charger->way, &task);
        } else if (MSG_STOP_CHARGE_FINISH == errnum)
        {
//                task.cmd = charger->wait_cmd;
                task.cid = charger->CID;
//                task.u.stop_charge.value = charger->stop_charge_value;
                task.errcode = MSG_STOP_CHARGE_FINISH;
                charger->wait_cmd = WAIT_CMD_NONE;
                finish_task_add(charger->way, &task);
        } else if (EMSG_START_CHARGE_ERROR == errnum)
        {
//                task.cmd = WAIT_CMD_START_CHARGE;
                task.cid = charger->CID;
                task.errcode = EMSG_START_CHARGE_ERROR;
                if (bf->recv_buff[9] == 1)
                {
                        task.errcode = 101;
                }
                bf->send_buff[0] = 0;
                for (i = 0; i < 16; i++)
                {
                        sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[10 + i]);
                }
                sprintf(bf->val_buff, "/PushMessage/pushMessage?cid=%08d\\\&type=%d\\\&code=%d\\\&id=%s", 
                        charger->CID, 3, task.errcode, bf->send_buff);
                if ((task.str = (char *)malloc(strlen(bf->val_buff) + 1)) != NULL)
                {
                       strcpy(task.str, bf->val_buff);
                }
                finish_task_add(charger->way, &task);
        } 
        else if (errnum == EMSG_START_CHARGE_CLEAN)
         {
                task.cid = charger->CID;
                task.errcode = EMSG_START_CHARGE_CLEAN;
                finish_task_add(SERVER_WAY, &task);
         } else if (errnum == EMSG_STOP_CHARGE_CLEAN)
         {
                task.cid = charger->CID;
                sprintf(bf->val_buff, "/PushMessage/pushMessage?cid=%08d\\\&type=%d\\\&code=%d\\\&id=%s", 
                                charger->CID, 3, 102, charger->uid);
                        if ( (task.str = (char *)malloc(strlen(bf->val_buff) + 1)) != NULL)
                        {
                                strcpy(task.str, bf->val_buff);
//                                task.cmd = WAIT_CMD_PUSH_MESSAGE_API;
//                                finish_task_add(SERVER_WAY, &task);
                        }
                task.cmd = MSG_CMD_FROM_CHARGER;
                task.errcode = EMSG_STOP_CHARGE_CLEAN;
                finish_task_add(SERVER_WAY, &task);

         }
        return;
    }
    debug_msg("Message: cid[%d], exception code: %d  ", charger->CID, errnum);

    if        (ESERVER_API_ERR   == errnum)
    {
    
    } else if ( ESERVER_CRC_ERR  == errnum)
    {
    
    } else if (EMSG_CHAOBIAO_ERROR == errnum)
    {
        
    } else if (EMSG_UPDATE_ERROR   == errnum)
    {
        
    } else if (EMSG_YUYUE_ERROR    == errnum)
    {
    
    } else if (EMSG_CONFIG_ERROR   == errnum)
    {
    
    } else if (EMSG_CONTROL_ERROR  == errnum)
    {
    
    } else if (EMSG_START_CHARGE_ERROR == errnum)
    {
                task.cid = charger->CID;
                task.errcode = EMSG_START_CHARGE_ERROR;
                if (bf->recv_buff[9] == 1)
                {
                        task.errcode = 101;
                }
                bf->send_buff[0] = 0;
                for (i = 0; i < 16; i++)
                {
                        sprintf(bf->send_buff + strlen(bf->send_buff), "%02x", bf->recv_buff[10 + i]);
                }
                sprintf(bf->val_buff, "/PushMessage/pushMessage?cid=%08d\\\&type=%d\\\&code=%d\\\&id=%s", 
                        charger->CID, 3, task.errcode, bf->send_buff);
//                task.errcode = 5; //bf->recv_buff[9];
                        if ( (task.str = (char *)malloc(strlen(bf->val_buff) + 1)) != NULL)
                        {
                                strcpy(task.str, bf->val_buff);
                                finish_task_add(charger->way, &task);
                        }
    } else if (EMSG_STOP_CHARGE_ERROR == errnum)
    {
                task.cid = charger->CID;
                task.errcode = EMSG_START_CHARGE_ERROR;
                finish_task_add(SERVER_WAY, &task);
    }

    return;
}    

#if 0
static void 
check_run_before(const CHARGER_INFO_TABLE *charger)
{
        if (charger->file_fd != 0)
            close(charger->file_fd);
        if (charger->file_name != NULL)
        {
            free(charger->file_name);
            charger->file_name = NULL;
        }
}
#endif
int 
have_wait_command(CHARGER_INFO_TABLE *charger, BUFF  *bf)
{
    struct join_wait_task    *wait = NULL;
    
    charger->present_cmd = bf->recv_buff[4]; // 当前命令
     if ( charger->present_cmd == 0x10)
     {
         charger->wait_cmd = WAIT_CMD_NONE;
         return 0;
     } 
     else if ( charger->present_cmd == 0x34)
     {
         // 可以发送任何命令
//          charger->wait_cmd = WAIT_CMD_NONE;
         if ( (wait = wait_task_remove_cid(charger->CID)) == NULL)
             return 0;
             // 如果有控制命令，直接丢弃
         charger->way = wait->way;
         switch (wait->cmd) 
         {
            case   WAIT_CMD_CONFIG:
                   charger->wait_cmd = WAIT_CMD_CONFIG;
                   charger->present_cmd = CHARGER_CMD_CONFIG_R;
            break;
            case  WAIT_CMD_CHAOBIAO:
            case  WAIT_CMD_ALL_CHAOBIAO:
                  charger->wait_cmd      = WAIT_CMD_CHAOBIAO; 
                  charger->present_cmd   = CHARGER_CMD_CHAOBIAO_R;
            break;
            
            case WAIT_CMD_ONE_UPDATE:  
            case WAIT_CMD_ALL_UPDATE:
                 charger->wait_cmd   = WAIT_CMD_ONE_UPDATE;
                 charger->present_cmd = CHARGER_CMD_START_UPDATE_R;
            break;
            case WAIT_CMD_YUYUE:
                  charger->wait_cmd = WAIT_CMD_YUYUE;
                  charger->present_cmd = CHARGER_CMD_YUYUE_R;          
            break;
            case WAIT_CMD_START_CHARGE:
                 charger->wait_cmd = WAIT_CMD_START_CHARGE;
                 charger->present_cmd = CHARGER_CMD_START_CHARGE_R;
            break;
            case WAIT_CMD_STOP_CHARGE:
                 //My_AES_CBC_Decrypt(charger->KEYB, bf->recv_buff+9, bf->recv_cnt-9, bf->send_buff);
                // debug_msg("present_mode:%#x", bf->send_buff[0]);
                 if (charger->present_status != CHARGER_CHARGING) // 在空闲的时候，处理扫码充电
                     goto err;
                 charger->wait_cmd = WAIT_CMD_STOP_CHARGE;
                 charger->present_cmd = CHARGER_CMD_STOP_CHARGE_R;
                 
            break;
            case WAIT_CMD_CTRL:
                  charger->wait_cmd = WAIT_CMD_CTRL;
                  charger->present_cmd = CHARGER_CMD_CTRL_R;
                    
            break;
            default:   // 控制命令，直接丢弃
                       
            break;
         }
     } else if (charger->present_cmd == 0x56)
     {
          charger->wait_cmd = WAIT_CMD_NONE;
         if ( (wait = wait_task_remove_cid(charger->CID)) == NULL)
             return 0;
         // 抄表跟电桩控制命令
          charger->way = wait->way;
         switch (wait->cmd)
         {
            case WAIT_CMD_CHAOBIAO:  // 抄表
                 charger->wait_cmd      = WAIT_CMD_CHAOBIAO; 
                 charger->present_cmd   = CHARGER_CMD_CHAOBIAO_R;
            break;
            case WAIT_CMD_CTRL: //停止充电 
                  charger->wait_cmd = WAIT_CMD_CTRL;
                  charger->present_cmd = CHARGER_CMD_CTRL_R;
            break;
            case WAIT_CMD_STOP_CHARGE:
                 charger->wait_cmd = WAIT_CMD_STOP_CHARGE;
                 charger->present_cmd = CHARGER_CMD_STOP_CHARGE_R;
                 
            break;
            default:
            break;
         }
        

     } else
     {
        // 处于更新或其他状态，什么都不做
     }  // end if
     if (wait != NULL)
        free(wait);
     return 0;
err:
     charger->wait_cmd = WAIT_CMD_NONE;
     if (wait != NULL)
         free(wait);
//     if (charger->uid != NULL)
//         free(charger->uid);
//     if (charger->package != NULL)
//         free(charger->package);
//     if (charger->start_charge_order != NULL)
//         free(charger->start_charge_order);
//     return 0;
}

int 
gernal_command(int fd, const int cmd, const CHARGER_INFO_TABLE *charger, BUFF *bf)
{
    int len, n, crc;
    time_t     tm;

//    memset(bf->val_buff, 0, strlen(bf->val_buff));
//    memset(bf->send_buff, 0, strlen(bf->send_buff));
    protocal_init_head(cmd, bf->send_buff, charger->CID);
    
    switch (cmd)
    {
        case  CHARGER_CMD_YUYUE_R:
              bf->send_buff[9]  = (char)charger->yuyue_time;
              bf->send_buff[10] = (char)(charger->yuyue_time >> 8);
              memcpy(bf->send_buff + 11, charger->uid, 32);
              n = 45;
//             free(charger->uid); 
       break;
      case CHARGER_CMD_HB_R:
            tm = time(0);
            bf->send_buff[9]  = (char)tm;
            bf->send_buff[10] = (char)(tm >> 8);
            bf->send_buff[11] = (char)(tm >> 16);
            bf->send_buff[12] = (char)(tm >> 24);
#if USE_POWER_BAR
            if (bf->recv_buff[4] == 0x56)
                bf->send_buff[13] = charger->real_current;
            else
                bf->send_buff[13] = charger_manager.power_bar_surpls_current;
#else
            bf->send_buff[13] = charger->model;//charger->support_max_current;
#endif
            n = 16;
      break; 
      case CHARGER_CMD_CONNECT_R:
            tm = time(0);
            memcpy(bf->send_buff + 9, bf->recv_buff + 9, 4); // ip
            bf->send_buff[13] = (char)tm;
            bf->send_buff[14] = (char)(tm >> 8);
            bf->send_buff[15] = (char)(tm >> 16);
            bf->send_buff[16] = (char)(tm >> 24);
            memcpy(bf->send_buff + 17, charger->KEYB, 16);
            n = 35;
      break;
      case CHARGER_CMD_CHARGE_REQ_R:
//            tm = time(0);
            bf->send_buff[9]  = (char)bf->real_time;
            bf->send_buff[10] = (char)(bf->real_time >>  8);
            bf->send_buff[11] = (char)(bf->real_time >> 16);
            bf->send_buff[12] = (char)(bf->real_time >> 24);
            bf->send_buff[13] = charger->target_mode;
            bf->send_buff[14] = charger->system_message;
            bf->send_buff[15] = bf->recv_buff[32];  // duration
            bf->send_buff[16] = bf->recv_buff[31];
            bf->send_buff[17] = bf->recv_buff[34];  // chargercode
            bf->send_buff[18] = bf->recv_buff[33]; 
            memcpy(bf->send_buff + 19, bf->recv_buff + 36, 16); // uid
#if USE_POWER_BAR
            bf->send_buff[35] = charger->real_current;
#else
            bf->send_buff[35] = charger->model;
#endif
            n = 38;
      break;
      case CHARGER_CMD_STOP_REQ_R :
            tm = time(0);
            bf->send_buff[9]  = (char)tm;
            bf->send_buff[10] = (char)(tm >>  8);
            bf->send_buff[11] = (char)(tm >> 16);
            bf->send_buff[12] = (char)(tm >> 24);
            bf->send_buff[13] = charger->target_mode;
            bf->send_buff[14] = charger->system_message;
            memcpy(bf->send_buff + 15, bf->recv_buff + 39, 16); // uid
            n = 33;
      break;
      case CHARGER_CMD_START_UPDATE_R:
            bf->send_buff[9]  = charger->version[0];
            bf->send_buff[10] = charger->version[1];
            bf->send_buff[11] = (char)(charger->file_length);
            bf->send_buff[12] = (char)(charger->file_length >>  8);
            bf->send_buff[13] = (char)(charger->file_length >> 16);
            bf->send_buff[14] = (char)(charger->file_length >> 24);
            if ( (charger->file_length & 1023) == 0)
                bf->send_buff[15] = charger->file_length / 1024;
            else
                bf->send_buff[15] = charger->file_length / 1024 + 1;
            n = 18; 
      break;
      case  CHARGER_CMD_UPDATE_R:
            if (lseek(charger->file_fd, bf->recv_buff[11] * 1024, SEEK_SET) < 0)
                goto err;
            if ( ( n = read(charger->file_fd, bf->val_buff, 1024)) <= 0)
                goto err;
            printf("read update_cnt:%d\n", n);
            bf->send_buff[9]  = (char)charger->version[0];
            bf->send_buff[10] = (char)charger->version[1];
            bf->send_buff[11] = (char)n;
            bf->send_buff[12] = (char)(n >> 8);
            bf->send_buff[13] = (char)(n >> 16);
            bf->send_buff[14] = (char)(n >> 24);
            bf->send_buff[15] = (char)bf->recv_buff[11];
            memcpy(bf->send_buff + 16, bf->val_buff, n);
            n += 18;
      break;
      case  CHARGER_CMD_CONFIG_R:
            if (lseek(charger->file_fd, charger->config_num * 1024, SEEK_SET) < 0)
                goto err;
            if ( (n = read(charger->file_fd, bf->val_buff, 1024)) <= 0)
                goto err;
            if (charger->file_length & 1023 == 0)
                bf->send_buff[9] = charger->file_length / 1024;
            else
                bf->send_buff[9] = charger->file_length / 1024 + 1;
            bf->send_buff[10] = (char)charger->file_length;
            bf->send_buff[11] = (char)(charger->file_length >> 8);
            bf->send_buff[12] = (char)(charger->file_length >> 16);
            bf->send_buff[13] = (char)(charger->file_length >> 24);
            bf->send_buff[14] = (char)n;
            bf->send_buff[15] = (char)(n >> 8);
            bf->send_buff[16] = (char)(n >> 16);
            bf->send_buff[17] = (char)(n >> 24);
            bf->send_buff[18] = (char)(charger->config_num);
            memcpy(bf->send_buff + 19, bf->val_buff, n);
            n += 21;
      break;
      case  CHARGER_CMD_CHAOBIAO_R:
            tm = time(0);
            bf->send_buff[9]  = (char)tm;
            bf->send_buff[10] = (char)(tm >> 8);
            bf->send_buff[11] = (char)(tm >> 16);
            bf->send_buff[12] = (char)(tm >> 24);
            bf->send_buff[13] = (char)(charger->cb_target_id);
            bf->send_buff[14] = (char)(charger->cb_target_id >> 8);
            bf->send_buff[15] = (char)(charger->cb_start_time);
            bf->send_buff[16] = (char)(charger->cb_start_time >> 8);
            bf->send_buff[17] = (char)(charger->cb_start_time >> 16);
            bf->send_buff[18] = (char)(charger->cb_start_time >> 24);
            bf->send_buff[19] = (char)(charger->cb_end_time);
            bf->send_buff[20] = (char)(charger->cb_end_time >> 8);
            bf->send_buff[21] = (char)(charger->cb_end_time >> 16);
            bf->send_buff[22] = (char)(charger->cb_end_time >> 24);
            bf->send_buff[23] = (char)(charger->cb_charging_code);
            bf->send_buff[24] = (char)(charger->cb_charging_code >> 8);
            n = 27;
      break;
      case  CHARGER_CMD_CTRL_R:
            tm = time(0);
            bf->send_buff[9]  = (char)tm;
            bf->send_buff[10] = (char)(tm >> 8);
            bf->send_buff[11] = (char)(tm >> 16);
            bf->send_buff[12] = (char)(tm >> 24);
            bf->send_buff[13] = charger->control_cmd;
            n = 16;
      break;
      case  CHARGER_CMD_START_CHARGE_R:
            memcpy(bf->send_buff + 9, charger->start_charge_order, 25);
            bf->send_buff[34] = (char)charger->start_charge_current;
            bf->send_buff[35] = (char)(charger->start_charge_current >> 8);
            strncpy(bf->send_buff + 36, charger->start_charge_package, 15);
            sprintf(bf->val_buff, "%04d", charger->start_charge_energy);
            memcpy(bf->send_buff + 51, bf->val_buff, 4);
            strncpy(bf->send_buff + 55, charger->uid, 32);
            n = 89;
//            free(charger->start_charge_order);
//            free(charger->start_charge_package);
//            free(charger->uid);
      break;
      case  CHARGER_CMD_STOP_CHARGE_R:
            memcpy(bf->send_buff + 9, charger->uid, 32);
            debug_msg("send_uid:%s\n", charger->uid);
            bf->send_buff[41] = charger->stop_charge_username;
            n = 44;
//            free(charger->uid);
      break;
      default:
            goto err;
      break;
    }

    crc = getCRC(bf->send_buff, n - 2);
    bf->send_buff[n - 2] = (char)crc;
    bf->send_buff[n - 1] = (char)(crc >> 8);
    Padding(bf->send_buff + 9, n - 9, bf->val_buff, &len);
    if (cmd == CHARGER_CMD_CONNECT_R)
        My_AES_CBC_Encrypt(KEYA, bf->val_buff, len, bf->send_buff + 9);
    else
        My_AES_CBC_Encrypt(charger->KEYB, bf->val_buff, len, bf->send_buff + 9);
//     if (writeable_timeout(fd, 4) <= 0)
//          return -1;
     if (write(fd, bf->send_buff, len + 9) != len + 9)
        goto err;
    return 0;
err:
    if (charger)
           debug_msg("Message: cid[%d], exception command: %d  ", charger->CID, cmd);
    return -1;
}
static void 
protocal_init_head(unsigned char CMD,  char *p_str, unsigned int cid)
{
	unsigned char len =0;
	strncpy(p_str, "EV<", 3);
#if 0
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
#endif 
	p_str[3] = 0;  //长度设置为0
	p_str[4] = CMD;
	p_str[5] = (unsigned char)(cid);
	p_str[6] = (unsigned char)(cid>>8);
	p_str[7] = (unsigned char)(cid>>16);
	p_str[8] = (unsigned char)(cid>>24);
}
