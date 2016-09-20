

#include "./include/protocal_charger.h"
#include "./include/AES.h"
#include "./include/CRC.h"
#include "./include/err.h"


void SendServer_charger_status(CHARGER_INFO_TABLE *charger, BUFF *bf)
{
        
        unsigned char   new_mode = 0, cmd = bf->recv_buff[4];
        char     *val;

        if ( cmd == CHARGER_CMD_HB )
            new_mode = bf->recv_buff[9];
        else if ( cmd == CHARGER_CMD_CHARGE_REQ || cmd == CHARGER_CMD_STATE_UPDATE || cmd == CHARGER_CMD_STOP_REQ)
            new_mode = bf->recv_buff[17];
            
//        printf("recv_cmd:%#x, cmd:%#x, present_mode:%d, new_mode:%d\n", bf->recv_buff[4], cmd, charger->present_mode, new_mode);

        if ( new_mode ==0 ||  new_mode == charger->present_mode )
            return ;
       
        debug_msg("检查状态有没有改变, CID[%d] ...", charger->CID); 
        if ( charger->present_mode > 0 &&  (( val = (char *)calloc(100, sizeof(char))) != NULL) )
        {
            sprintf(val, "/ChargerState/changesStatus?key={cid:\\\"%08d\\\",status:%d}", charger->CID, new_mode);
#if FORMAL_ENV
            cmd_frun("dashboard url_post 10.9.8.2:8080/ChargerAPI %s", val);
#else
            cmd_frun("dashboard url_post 10.9.8.2:8080/test %s", val);
#endif
            debug_msg("发送改变状态, CID[%d], %s ...", charger->CID, val);
            free(val);
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
error_hander(int handle, CHARGER_INFO_TABLE *charger, BUFF *bf, int errnum)
{
    struct finish_task    task;
    int     i;
    // 操作成功
    if ( !handle )
    {
//        printf("本次通信成功:CID[%d] ...\n", *(unsigned int *)(bf->recv_buff + 5));
        if (errnum == ESERVER_SEND_SUCCESS)
         { 
                if (bf->recv_buff[4] != 0x10) 
                    SendServer_charger_status(charger, bf);
                return;
         }
        
        if (errnum == ECHAOBIAO_FINISH)     //抄表完成
        {
             task.cmd = charger->wait_cmd;
             task.cid = charger->CID;
             task.errcode = ECHAOBIAO_FINISH;
             task.u.chaobiao.start_time = charger->cb_start_time;
             task.u.chaobiao.end_time = charger->cb_end_time;
             task.u.chaobiao.chargercode = *(unsigned short *)(bf->recv_buff +984);
             close(charger->file_fd);
             charger->wait_cmd = WAIT_CMD_NONE;
             finish_task_add(charger->way, &task);
        
        } else if (errnum == EUPDATE_FINISH) // 更新软件完成
        {
             task.cmd = charger->wait_cmd;
             task.cid = charger->CID;
             task.errcode = EUPDATE_FINISH;
             close(charger->file_fd);
             charger->wait_cmd = WAIT_CMD_NONE;
             finish_task_add(charger->way, &task);
        
        }else if (ECONFIG_FINISH == errnum) // 推送配置完成
        {
               task.cmd = charger->wait_cmd;
               task.cid = charger->CID;
               task.errcode = ECHAOBIAO_FINISH;
               close(charger->file_fd); 
               charger->wait_cmd = WAIT_CMD_NONE;
               sprintf(bf->val_buff, "%s/%s/%d%c", WORK_DIR, CONFIG_DIR, charger->CID, '\0');
               unlink(bf->val_buff);
               finish_task_add(charger->way, &task);
        } else if (EYUYUE_FINISH == errnum)
        {
               task.cmd = charger->wait_cmd;
               task.cid = charger->CID;
               task.errcode = CHARGER_RESERVED;
               charger->wait_cmd = WAIT_CMD_NONE;
               bf->val_buff[0] = '\0';
               for (i = 0; i < 16; i++)
               {
                    sprintf(bf->val_buff + strlen(bf->val_buff), "%02x", bf->recv_buff[9 + i]);
               }
               strncpy(task.u.yuyue.uid, bf->val_buff, 32);
               finish_task_add(charger->way, &task);
        } else if (ECONTROL_FINISH == errnum)
        {
                task.cmd = charger->wait_cmd;
                task.cid = charger->CID;
                task.u.control.status = 1;
                charger->wait_cmd = WAIT_CMD_NONE;
                finish_task_add(charger->way, &task);    
        } else if (ECONTROL_NO_CTRL == errnum)
        {
                task.cmd = charger->wait_cmd;
                task.cid = charger->CID;
                task.u.control.status = 0;
                charger->wait_cmd = WAIT_CMD_NONE;
                finish_task_add(charger->way, &task);    
        } else if (ESTART_CHARGE_FINISH == errnum)
        {
                task.cmd = charger->wait_cmd;
                task.cid = charger->CID;
                task.errcode = ESTART_CHARGE_FINISH;
                charger->wait_cmd = WAIT_CMD_NONE;
                finish_task_add(charger->way, &task);
        } else if (ESTOP_CHARGE_FINISH == errnum)
        {
                task.cmd = charger->wait_cmd;
                task.cid = charger->CID;
                task.u.stop_charge.value = charger->stop_charge_value;
                task.errcode = ESTOP_CHARGE_FINISH;
                charger->wait_cmd = WAIT_CMD_NONE;
                finish_task_add(charger->way, &task);
        }
        return;
    }

    debug_msg("操作IO出错,CID[%d], errcode:%d ...",  *(unsigned int *)(bf->recv_buff +5), errnum);
    printf("操作IO出错,CID[%d], errcode...",  *(unsigned int *)(bf->recv_buff + 5), errnum);
    // 操作失败代码部分

    if        (ESERVER_API_ERR   == errnum)
    {
    
    } else if ( ESERVER_CRC_ERR  == errnum)
    {
    
    } else if (ECHAOBIAO_API_ERR == errnum)
    {
        
    } else if (EUPDATE_API_ERR   == errnum)
    {
        
    } else if (EYUYUE_API_ERR    == errnum)
    {
    
    } else if (ECONFIG_API_ERR   == errnum)
    {
    
    } else if (ECONTROL_API_ERR  == errnum)
    {
    
    } else if (ESTART_CHARGE_API_ERR == errnum)
    {
    
    } else if (ESTOP_CHARGE_API_ERR == errnum)
    {
    
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
    struct wait_task    *wait = NULL;
    struct stat         *st = NULL;
    
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
                   sprintf(bf->val_buff, "%s/%s/%s", WORK_DIR, CONFIG_DIR, wait->u.config.name);
                   if ( access(bf->val_buff, F_OK) != 0)
                   {
                       // 设置错误代码 
                       debug_msg("access %s failed ...", bf->val_buff);
                        goto err;
                   }
                   if ( (st = (struct stat *)malloc(sizeof(struct stat))) == NULL)
                   {
                        debug_msg("malloc (struct stat) failed ...");
                        goto err;
                   }
                   if ( (charger->file_fd = open(bf->val_buff, O_RDONLY, 0444)) < 0)
                   {
                        debug_msg("open %s failed ...", bf->val_buff);
                        goto err;
                   }
                   if (fstat(charger->file_fd, st) < 0)
                   {
                        close(charger->file_fd);
                        debug_msg("fstat %s failed ...", bf->val_buff);   
                        goto err;
                   }
                   debug_msg("接收到推送配置命令:CID[%08d] %s ...", charger->CID, bf->val_buff);
                   charger->wait_cmd = WAIT_CMD_CONFIG;
                   charger->file_length = st->st_size;
                   charger->present_cmd = CHARGER_CMD_CONFIG_R;
            break;
            case  WAIT_CMD_CHAOBIAO:
                  if (wait->way == SERVER_WAY)
                      sprintf(bf->val_buff, "%s/%s/%08d_server", WORK_DIR, CHAOBIAO_DIR, wait->cid);  
                   else
                      sprintf(bf->val_buff, "%s/%s/%08d_web", WORK_DIR, CHAOBIAO_DIR, wait->cid); 
                  if ( (charger->file_fd = creat(bf->val_buff, FILEPERM)) < 0)
                  {
                        debug_msg("create %s failed ...", bf->val_buff);
                        goto err; 
                  } 
                 debug_msg("接收到抄表命令:CID[%08d] ...", charger->CID);
                  charger->cb_start_time = wait->u.chaobiao.start_time;
                  charger->cb_end_time   = wait->u.chaobiao.end_time; 
                  charger->wait_cmd      = WAIT_CMD_CHAOBIAO; 
                  charger->present_cmd   = CHARGER_CMD_CHAOBIAO_R;
            break;
            
            case WAIT_CMD_ONE_UPDATE:  
            case WAIT_CMD_ALL_UPDATE:
                 sprintf(bf->val_buff, "%s/%s/%s", WORK_DIR, UPDATE_DIR, wait->u.update.name);
                 if ( access(bf->val_buff, F_OK) != 0)
                 {
                    debug_msg("access %s failed ...", bf->val_buff);
                    goto err;
                 }
                 if ( (st = (struct stat *)malloc(sizeof(struct stat))) == NULL)
                 {
                    debug_msg("malloc %s failed ...", bf->val_buff);
                    goto err;
                 }
                 if (stat(bf->val_buff, st) < 0)
                 {
                    debug_msg("stat %s failed ...", bf->val_buff);
                    goto err;
                 }
                 if ( (charger->file_fd = open(bf->val_buff, O_RDONLY, 0444)) < 0)
                 {
                    debug_msg("open %s failed ...", bf->val_buff);
                    goto err;
                 }
                 debug_msg("接收到更新文件命令:CID[%08d] %s ...", charger->CID, bf->val_buff);
                 charger->file_length = st->st_size;
                 charger->version[0]  = wait->u.update.version[0];
                 charger->version[1]  = wait->u.update.version[1];
                 if (charger->wait_cmd == WAIT_CMD_ONE_UPDATE)
                    charger->wait_cmd    = WAIT_CMD_ONE_UPDATE;
                 else
                     charger->wait_cmd   = WAIT_CMD_ALL_UPDATE;
                 charger->present_cmd = CHARGER_CMD_START_UPDATE_R;
            break;
            case WAIT_CMD_YUYUE:
                  debug_msg("接收到预约命令:CID[%08d] ...", charger->CID);
                  if ( (charger->uid = (char *)malloc(32)) == NULL)
                  {
                       debug_msg("yuyue malloc failed, CID[%d] ...", charger->CID);
                       goto err;
                  }
                  charger->yuyue_time = wait->u.yuyue.time;
                  memcpy(charger->uid, wait->u.yuyue.uid, 32); 
                  charger->wait_cmd = WAIT_CMD_YUYUE;
                  charger->present_cmd = CHARGER_CMD_YUYUE_R;          
            break;
            case WAIT_CMD_START_CHARGE:
                 debug_msg("接收到开始充电命令:CID[%08d] ...", charger->CID);
                 if ( (charger->start_charge_order = (char *)malloc(sizeof(wait->u.start_charge.order_num) )) == NULL)
                 {
                    debug_msg("start charge malloc failed, CID[%d] ...", charger->CID);
                    goto err;
                 }
                 if ( (charger->start_charge_package = (char *)malloc(sizeof(wait->u.start_charge.package))) == NULL)
                 {
                    free(charger->start_charge_order);
                    debug_msg("start charge malloc failed, CID[%d] ...", charger->CID);
                    goto err;
                 }
                 if ( (charger->uid = (char *)malloc(sizeof(wait->u.start_charge.uid) )) == NULL)
                 {
                    free(charger->start_charge_order);
                    free(charger->start_charge_package);
                    debug_msg("start charge malloc failed, CID[%d] ...", charger->CID);
                    goto err;
                 }
                 memcpy(charger->start_charge_order, wait->u.start_charge.order_num, sizeof(wait->u.start_charge.order_num) );
                 memcpy(charger->start_charge_package, wait->u.start_charge.package, sizeof(wait->u.start_charge.package));
                 memcpy(charger->uid, wait->u.start_charge.uid, sizeof(wait->u.start_charge.uid));
                 charger->start_charge_energy = wait->u.start_charge.energy;
                 charger->start_charge_current = wait->u.start_charge.current;
                 charger->wait_cmd = WAIT_CMD_START_CHARGE;
                 charger->present_cmd = CHARGER_CMD_START_CHARGE_R;
            break;
            case WAIT_CMD_STOP_CHARGE:
                 My_AES_CBC_Decrypt(charger->KEYB, bf->recv_buff+9, bf->recv_cnt-9, bf->send_buff);
                 debug_msg("present_mode:%#x", bf->send_buff[0]);
                 if (bf->send_buff[0] != CHARGER_CHARGING) // 在空闲的时候，处理扫码充电
                     goto err;
                 debug_msg("接收到停止充电命令:CID[%08d] ...", charger->CID);
                 if ( (charger->uid = (char *)malloc(sizeof(wait->u.stop_charge.uid) )) == NULL)
                  {
                        debug_msg("stop charge malloc failed, CID[%d] ...", charger->CID);
                        goto err;
                  }
                 memcpy(charger->uid, wait->u.stop_charge.uid, sizeof(wait->u.stop_charge.uid));
                 charger->wait_cmd = WAIT_CMD_STOP_CHARGE;
                 charger->present_cmd = CHARGER_CMD_STOP_CHARGE_R;
                 
            break;
            case WAIT_CMD_CTRL:
                  debug_msg("接收到控制命令:CID[%08d] ...", charger->CID);
                  charger->wait_cmd = WAIT_CMD_CTRL;
                  charger->control_cmd =  wait->u.control.value; 
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
                  if (wait->way == SERVER_WAY)
                      sprintf(bf->val_buff, "%s/%s/%d_server", WORK_DIR, CHAOBIAO_DIR, charger->CID);  
                   else
                      sprintf(bf->val_buff, "%s/%s/%d_web", WORK_DIR, CHAOBIAO_DIR, charger->CID); 
                  printf("%s\n", bf->val_buff);
                   if ( (charger->file_fd = creat(bf->val_buff, FILEPERM)) < 0)
                  {
                        debug_msg("create %s failed ...", bf->val_buff);
                        goto err; 
                  } 
                 debug_msg("接收到抄表命令:CID[%08d] ...", charger->CID);
                 charger->cb_start_time = wait->u.chaobiao.start_time;
                 charger->cb_end_time   = wait->u.chaobiao.end_time; 
                 charger->wait_cmd      = WAIT_CMD_CHAOBIAO; 
                 charger->present_cmd   = CHARGER_CMD_CHAOBIAO_R;
            break;
            case WAIT_CMD_CTRL: //停止充电 
                  debug_msg("接收到控制命令:CID[%08d] ...", charger->CID);
                  charger->wait_cmd = WAIT_CMD_CTRL;
                  charger->control_cmd =  wait->u.control.value; 
                  charger->present_cmd = CHARGER_CMD_CTRL_R;
            break;
            case WAIT_CMD_STOP_CHARGE:
                 debug_msg("接收到停止充电命令:CID[%08d] ...", charger->CID);
                 if ( (charger->uid = (char *)malloc(sizeof(wait->u.stop_charge.uid) )) == NULL)
                  {
                        debug_msg("stop charge malloc failed, CID[%d] ...", charger->CID);
                        goto err;
                  }
                 memcpy(charger->uid, wait->u.stop_charge.uid, sizeof(wait->u.stop_charge.uid));
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
     if (st != NULL)
        free(st);
     if (wait != NULL)
        free(wait);
     return 0;
err:
     charger->wait_cmd = WAIT_CMD_NONE;
     if (wait != NULL)
         free(wait);
     if (st != NULL)
         free(st);
     return 0;
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
             free(charger->uid); 
       break;
      case CHARGER_CMD_HB_R:
            tm = time(0);
            bf->send_buff[9]  = (char)tm;
            bf->send_buff[10] = (char)(tm >> 8);
            bf->send_buff[11] = (char)(tm >> 16);
            bf->send_buff[12] = (char)(tm >> 24);
            bf->send_buff[13] = charger->support_max_current;
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
            tm = time(0);
            bf->send_buff[9]  = (char)tm;
            bf->send_buff[10] = (char)(tm >>  8);
            bf->send_buff[11] = (char)(tm >> 16);
            bf->send_buff[12] = (char)(tm >> 24);
            bf->send_buff[13] = charger->target_mode;
            bf->send_buff[14] = charger->system_message;
            bf->send_buff[15] = bf->recv_buff[32];  // duration
            bf->send_buff[16] = bf->recv_buff[31];
            bf->send_buff[17] = bf->recv_buff[34];  // chargercode
            bf->send_buff[18] = bf->recv_buff[33]; 
            memcpy(bf->send_buff + 19, bf->recv_buff + 35, 16); // uid
            bf->send_buff[35] = charger->model;
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
            free(charger->start_charge_order);
            free(charger->start_charge_package);
            free(charger->uid);
      break;
      case  CHARGER_CMD_STOP_CHARGE_R:
            memcpy(bf->send_buff + 9, charger->uid, 32);
            n = 43;
            free(charger->uid);
      break;
      default:
            debug_msg("没有处理的命令, CID[%#x] ...", charger->CID);
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
    if (write(fd, bf->send_buff, len + 9) != len + 9)
        goto err;
    printf("发送成功, 命令:%#x, CID[%d] ...\n", cmd, charger->CID);
    return 0;
err:
    printf("发送失败, 命令:%#x, CID[%d] failed ...\n", cmd, charger->CID);
    return -1;
}

// 协议函数化
#if 0
int  ev_conn_req(CHARGER_INFO_TABLE *have_charger, unsigned char  *recv_buff, int recvlen)
{
    char    key[16], val_buff[20];

    key_init(key);
    if ( have_charger )
    {
        
				//将改变的IP存入数据库
				sprintf(bf->val_buff, "%d.%d.%d.%d", bf->recv_buff[9], bf->recv_buff[10], bf->recv_buff[11], bf->recv_buff[12]);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.IP", have_charger->tab_name) < 0)	// 存入IP
                    return  -1;
                memcpy(bf->val_buff, key_addr, 16);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.KEYB", have_charger->tab_name) < 0)
                    return  -1;
				strncpy(bf->val_buff, bf->recv_buff+18, 10);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Model", have_charger->tab_name) < 0) // series
                    return -1;
				strncpy(bf->val_buff, bf->recv_buff+28, 10);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.Series", have_charger->tab_name) < 0)
                    return -1;
                sprintf(bf->val_buff, "%d", bf->recv_buff[42]);
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerType", have_charger->tab_buff) < 0)	
                    return -1;
				sprintf(bf->val_buff, "%d.%02d%c", bf->recv_buff[41], bf->recv_buff[40], '\0');	//ChargerVersion
				if (ev_uci_save_action(UCI_SAVE_OPT, true, bf->val_buff, "chargerinfo.%s.ChargerVersion", charger_charger->tab_name) < 0)
                    return -1;
				memcpy(have_charger->KEYB, key, 16);
                have_charger->wait_cmd = WAIT_CMD_NONE;
				strncpy(bf->val_buff, bf->recv_buff+18, 10);
				if(!strcmp(bf->val_buff, "EVG-16N"))
				{
					have_charger->model = EVG_16N;
				}else if(!strcmp(bf->val_buff, "EVG-32N"))
				{
					have_charger->model = EVG_32N;	
				}else if(!strcmp(bf->val_buff, "EVG-32NW"))
				{
					have_charger->model = EVG_32N;	
				}else if(!strcmp(bf->val_buff, "EVG-32N"))
				{
					have_charger->model = EVG_32N;	
				}
                return 0;
    } else
    {
        
    }
}

#endif
