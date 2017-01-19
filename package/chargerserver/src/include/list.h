#ifndef _M_LIST_H
#define _M_LIST_H

#include "serv_config.h"


extern struct finish_task *finish_head;
extern struct finish_task *finish_tail;
extern pthread_mutex_t  finish_task_lock;
extern pthread_cond_t  finish_task_cond;

void finish_task_add(int , const CHARGER_INFO_TABLE *, const char *, int );
struct finish_task *finish_task_remove_cid(int );
void finish_task_remove_head(struct finish_task *);


#endif      // list.h


