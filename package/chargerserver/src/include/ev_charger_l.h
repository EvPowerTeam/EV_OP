
#ifndef __EV_CHARGER_L_HH
#define __EV_CHARGER_L_HH

#include "serv_config.h"

typedef struct client_info
{
        int fd;
        unsigned int cid;
}CLIENT_INFO;

typedef struct ev_proto_ll{
        int  cmd_type;
        int (*cmd_run)(int fd, BUFF *bf, CHARGER_INFO_TABLE *charger);        

}ev_proto_l;

int do_serv_run(int, int , BUFF *, CHARGER_INFO_TABLE *, void (*)(int, CHARGER_INFO_TABLE*,BUFF *,int));

#endif


