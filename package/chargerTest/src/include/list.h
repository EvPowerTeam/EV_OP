#ifndef _M_LIST_H
#define _M_LIST_H

#include "serv_config.h"


extern struct wait_task *wait_head;
extern struct wait_task *wait_tail;
extern struct finish_task *finish_head;
extern struct finish_task *finish_tail;
extern pthread_mutex_t  wait_task_lock;
extern pthread_mutex_t  finish_task_lock;
extern pthread_cond_t  finish_task_cond;

void finish_task_add(int , struct finish_task *);
struct finish_task *finish_task_remove_cid(unsigned int );
void finish_task_remove_head(struct finish_task *);
void  wait_task_add(int , struct wait_task *);
struct wait_task *wait_task_remove_cid(unsigned int );
struct wait_task *wait_task_remove_cmd(unsigned int , int );



#endif      // list.h


