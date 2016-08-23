
#ifndef _PRO_CHARGER_H
#define _PRO_CHARGER_H

#include  "serv_config.h"


int
have_wait_command(CHARGER_INFO_TABLE *, BUFF *);

int  
gernal_command(int, const int, const CHARGER_INFO_TABLE *, BUFF *);

void 
err_hander(int, CHARGER_INFO_TABLE *, BUFF *, int);

#endif // protocal_charger.h

