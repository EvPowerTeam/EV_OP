
#include "./include/list.h"


struct  join_wait_task   *wait_head = NULL;
struct  join_wait_task   *wait_tail = NULL;
pthread_mutex_t          wait_task_lock  = PTHREAD_MUTEX_INITIALIZER;    //保护待处理命令队列锁
struct  finish_task     *finish_head = NULL;
struct  finish_task     *finish_tail = NULL;
pthread_mutex_t     finish_task_lock = PTHREAD_MUTEX_INITIALIZER;   //保护已经完成命令队列锁
pthread_cond_t      finish_task_cond = PTHREAD_COND_INITIALIZER;   //保护已经完成命令队列锁


static struct  finish_task *finish_task_create(int way, struct finish_task *data)
{
    struct  finish_task     *task;

    if ( (task = (struct finish_task *)malloc(sizeof(struct finish_task))) == NULL)
        return NULL;
//    *task = *data;    
    memcpy(task, data, sizeof(struct finish_task));
    task->way = way;
    task->next = NULL;
    task->pre_next = NULL;
    return task;
}

void finish_task_add(int way, struct finish_task *cmd)
{
    struct finish_task  *task;

    if ( (task = finish_task_create(way, cmd)) == NULL)
    {
        printf("return null ...\n");
        return ;
    }
    pthread_mutex_lock(&finish_task_lock);
    task->pre_next = finish_tail;
    if (finish_tail == NULL)
    {
        finish_head = task;
        finish_tail = task;
    }
    else
    {
        finish_tail->next = task;
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

// create a struct join_wait_task node
static struct  join_wait_task *wait_task_create(int way, struct join_wait_task *data)
{
    struct  join_wait_task   *task;

    if ( (task = (struct join_wait_task *)malloc(sizeof(struct join_wait_task))) == NULL)
        return NULL;
     // initialize
//    *task = *data;
    memcpy(task, data, sizeof(struct join_wait_task));
    task->way = way;
    task->next = NULL;
    task->pre_next = NULL;
    return task;
}
void  wait_task_add(int way, struct join_wait_task *cmd)
{
    struct join_wait_task  *task;
    
    task = wait_task_create(way, cmd); 
    pthread_mutex_lock( &wait_task_lock);
    task->pre_next = wait_tail;
    if (wait_tail == NULL)
    {
        wait_head = task;
        wait_tail = task;
    }
    else
    {
        wait_tail->next = task;
        wait_tail = task;
    }
    pthread_mutex_unlock(&wait_task_lock);
}

struct join_wait_task *wait_task_remove_cid(int cid)
{
    struct join_wait_task *task = NULL;
    
    pthread_mutex_lock(&wait_task_lock);
    task = wait_head;
    while(task)
    {
        if (task->cid == cid)
        {
            if (task->next == NULL)
                wait_tail = task->pre_next;
            else
                task->next->pre_next = task->pre_next;
            
            if (task->pre_next == NULL)
                wait_head = task->next;
            else
               task->pre_next->next = task->next;
            break;
        }
        task = task->next;
    }
    pthread_mutex_unlock(&wait_task_lock);
    return task;
}

struct join_wait_task *wait_task_remove_cmd(int cid, int cmd)
{
    struct join_wait_task *task = NULL;
    printf("aa\n");
    pthread_mutex_lock(&wait_task_lock);
    printf("aaa\n");
    task = wait_head;
    while (task)
    {
        printf("aaaa\n");
        if (task->cmd == cmd && task->cid == cid)
        {
            if (task->next == NULL)
                wait_tail = task->pre_next;
            else
                task->next->pre_next = task->pre_next;
            if (task->pre_next == NULL)
                wait_head = task->next;
            else
               task->pre_next->next = task->next;
            break;
        }
        task = task->next;
    }
   pthread_mutex_unlock(&wait_task_lock);
   return task;
}


