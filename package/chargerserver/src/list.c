
#include "./include/list.h"


struct  finish_task     *finish_head = NULL;
struct  finish_task     *finish_tail = NULL;
pthread_mutex_t     finish_task_lock = PTHREAD_MUTEX_INITIALIZER;   //保护已经完成命令队列锁
pthread_cond_t      finish_task_cond = PTHREAD_COND_INITIALIZER;   //保护已经完成命令队列锁


static struct  finish_task *finish_task_create(void)
{
    struct  finish_task     *task;

    if ( (task = (struct finish_task *)malloc(sizeof(struct finish_task))) == NULL)
        return NULL;    
    task->next = NULL;
    task->pre_next = NULL;

    return task;
}


void finish_task_add(int cmd, const CHARGER_INFO_TABLE *charger, const char *str, int errcode)
{
    struct finish_task  *task;

    if ( (task = finish_task_create()) == NULL)
    {
        printf("return null ...\n");
        if (str)
            free(str);
        return ;
    }
    task->cmd = cmd;
    task->charger = charger;
    task->str = str;
    task->errcode = errcode;
    pthread_mutex_lock(&finish_task_lock);
    if (finish_tail == NULL)
    {
        finish_head = task;
        finish_tail = task;
    }
    else
    {
        finish_tail->next = task;
        task->pre_next = finish_tail;
        finish_tail = task;
    }
    pthread_mutex_unlock(&finish_task_lock);
    pthread_cond_signal(&finish_task_cond);
}

struct finish_task *finish_task_remove_cid(int cid)
{
    struct finish_task *task = NULL;

    pthread_mutex_lock(&finish_task_lock);
    task = finish_head;
    while(task)
    {
        if (task->cid == cid)
        {
            if (task->next == NULL)
                finish_tail = task->pre_next;
            else
                task->next->pre_next = task->pre_next;
            if (task->pre_next == NULL)
                finish_head = task->next;
            else
               task->pre_next->next = task->next;
            break;
        }
        task = task->next;
    }
    pthread_mutex_unlock(&finish_task_lock);
    return task;
}

void finish_task_remove_head(struct finish_task *task)
{
    if (task->next == NULL)
        finish_tail = task->pre_next;
    else
        task->next->pre_next = task->pre_next;

    if (task->pre_next == NULL)
        finish_head = task->next;
    else
        task->pre_next->next = task->next;
}

