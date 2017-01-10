#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
//#include "./include/err.h"
//#include "./include/system_call.h"
//pthread handler function

#define   UPDATE_FILE   "abcd.bin"
void *pthread_func(void *arg)
{
    time_t tm;
    struct tm *tim;
    char    val_buff[200];
    tm = time(0);
    tim = localtime(&tm);
    char buf[] = UPDATE_FILE;
    sprintf(val_buff, "%s%s%c%s-%d-%d-%d%s", "mv ", UPDATE_FILE, ' ', strtok(buf, "."), tim->tm_year+1900, tim->tm_mon+1, tim->tm_mday, ".bin");
    system(val_buff);
    printf("%s\n", val_buff);
            return (void *)0;
}

typedef struct {
	unsigned char	total_num;		//充电装总个数，默认给8
	unsigned char	limit_max_current;	//限制的最大的充电电流
	unsigned char	present_charger_num;	//当前
	unsigned char	present_off_net_cnt;	//当前断网的个数
	unsigned char	present_networking_cnt;	//当前联网的总数
	unsigned char	present_charging_cnt;	//当前正在充电的个数
	unsigned int	present_record_cnt;	//当前充电记录个数
}CHARGER_MANAGER;
CHARGER_MANAGER		charger_manager = {
	.total_num = 8,
	.limit_max_current = 56
};

typedef struct A{
	int b;
	int c;
	int d;
}AA;
AA C= {.c = 100, .d = 200};


char *p_malloc;

void cleanup_pthread(void *arg)
{
    free(p_malloc);
    printf("clean\n");
}

void *pthread_main(void *arg)
{
    pthread_t   tid;
    int i = 0;

    while(1)
    {
        i++;
  //      usleep(1);
    }
}

pthread_mutex_t     one_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     two_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     first_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t     one_cond  = PTHREAD_COND_INITIALIZER;
pthread_cond_t     two_cond  = PTHREAD_COND_INITIALIZER;
int                 one;
int                 two;

void    *pthread_main_1(void *arg)
{
    while(1)
    {
        pthread_mutex_lock(&one_mutex);
        while(one)
         {
              pthread_cond_wait(&one_cond, &one_mutex);
              printf("aa\n");
        }
        printf("唤醒one\n");
        one = 1;
        pthread_mutex_lock(&two_mutex);
        pthread_mutex_unlock(&one_mutex);
        two = 0;
        pthread_cond_signal(&two_cond);
        printf("发信号two\n");
        pthread_mutex_unlock(&two_mutex);
    }
}

void    *pthread_main_2(void *arg)
{
    for ( ; ; )
    {
        two = 1;
        pthread_mutex_lock(&first_mutex);
        pthread_mutex_lock(&one_mutex);
        one = 0;
        pthread_cond_signal(&one_cond);
        printf("发信号one\n");
        pthread_mutex_unlock(&one_mutex);
        printf("one\n");
        pthread_mutex_lock(&two_mutex);
        while(two)
        {
              pthread_cond_wait(&two_cond, &two_mutex);
        }
        printf("唤醒two\n");
        two = 1;
        pthread_mutex_unlock(&two_mutex);
        pthread_mutex_unlock(&first_mutex);
        sleep(1);
    }

}
int my_strlen(const char *s)
{
    const char *s1 = s;
    while (*s != '\0' && *s++);

    return s- s1;
}


// 接收命令的数据结构
struct cb { time_t  start_time; time_t  end_time; };

typedef struct  {
    int          cmd;
    unsigned int cid;
    union  {
        struct  cb  chaobiao;
        char        name[20];
    }u;
}RECV_CMD;

// 待处理命令队列
struct  wait_task {
    int            way;        // 页面设置方式还是服务器设置方式
    RECV_CMD        wait_cmd;        
    struct  wait_task   *next;
    struct  wait_task   *pre_next;
};
struct  wait_task   *wait_head = NULL;
struct  wait_task   *wait_tail = NULL;
pthread_mutex_t     wait_task_lock  = PTHREAD_MUTEX_INITIALIZER;    //保护待处理命令队列锁

// 已经处理完成命令队列
struct  finish_task {
    int             way;
    int             cmd;
    int             err_code;
    unsigned int    cid;
    struct  finish_task *next;
};
struct  finish_task     *finish_head = NULL;
pthread_mutex_t     finish_task_lock = PTHREAD_MUTEX_INITIALIZER;   //保护已经完成命令队列锁

// create a struct wait_task node
struct  wait_task *wait_task_create(int way, RECV_CMD  cmd)
{
    struct  wait_task   *task;

    if ( (task = (struct wait_task *)malloc(sizeof(struct wait_task))) == NULL)
        printf("malloc failed");
    // initializer
    task->way = way;
    task->wait_cmd = cmd;
    task->next = NULL;
    task->pre_next = NULL;
    return task;
}

void  wait_task_add(int way, RECV_CMD cmd)
{
    struct wait_task  *task;
    
    task = wait_task_create(way, cmd); 
    pthread_mutex_lock(&wait_task_lock);
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
    pthread_mutex_lock(&wait_task_lock);
}

struct wait_task *wait_task_remove_cid(unsigned int cid)
{
    struct wait_task *task = NULL;
    struct wait_task *pre_task = NULL;

    pthread_mutex_lock(&wait_task_lock);
    task = wait_head;
    while(task)
    {
        if (task->wait_cmd.cid == cid)
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

struct wait_task *wait_task_remove_cmd(int cmd)
{
    struct wait_task *task = NULL;
    struct wait_task *pre_task = NULL;

    pthread_mutex_lock(&wait_task_lock);
    task = wait_head;
    while (task)
    {
        if (task->wait_cmd.cmd == cmd)
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

int
main(int argc, char **argv)
{
	pthread_t	tid;
   
    char    arr[5] = {1, 2, 3, 4, 5};

    RECV_CMD    cmd;
    cmd.cid = 0x7001;
    cmd.cmd = 0x11;

    wait_task_add(1, cmd);
    cmd.cid = 0x7002;
    cmd.cmd = 0x22;
    wait_task_add(2, cmd);
    cmd.cid = 0x7003;
    cmd.cmd = 0x33;
    wait_task_add(3, cmd);
    cmd.cid = 0x7004;
    cmd.cmd = 0x44;
    wait_task_add(4, cmd);
   char buff[10] ;
  memset(buff, 1, 10);
 strcpy(buff, "okay");
printf("%s\n", buff); 
    printf("%s\n", strchr("helell.okokok", '.')+1);
    open("/etc/passwd", 0022);
   printf("%s\n", strerror(errno));
   perror("open");

struct  wait_task *task;
    task = wait_task_remove_cmd(0x11);
    if (task != NULL)
         printf("cid = %#x\n", task->wait_cmd.cid);
    else
        printf("null\n");
    task = wait_task_remove_cmd(0x22);
    if (task != NULL)
         printf("cid = %#x\n", task->wait_cmd.cid);
    else
        printf("null\n");
//    task = wait_task_remove_cid(0x7002);
    task = wait_task_remove_cid(0x7004);
    if (task != NULL)
         printf("cid = %#x\n", task->wait_cmd.cid);
    else
        printf("null\n");
    task = wait_task_remove_cmd(0x33);
    if (task != NULL)
         printf("cid = %#x\n", task->wait_cmd.cid);
    else
       printf("null\n");
//    free(task);
//    task = NULL;
//    printf("printf:");
//    while (wait_head)
//    {
//        printf("way = %d\n", wait_head->way);
//        free(wait_head);
//        wait_head = wait_head->next;
//    }
    if (wait_head || wait_tail)
    {
        printf("no null\n");
    }else if (wait_head== NULL && wait_tail == NULL)
    {
        printf("null...\n");
    }
    while (wait_tail)
    {
        printf("way2:%d\n", wait_tail->way);
        free(wait_tail);
        wait_tail = wait_tail->pre_next;     
    }
//    pthread_create(&tid, NULL, pthread_main_1, NULL);
//    pthread_create(&tid, NULL, pthread_main_2, NULL);
//    pthread_create(&tid, NULL, pthread_main, NULL);
//    pthread_create(&tid, NULL, pthread_main_1, NULL);
//    for( ; ;)
//	{
//		sleep(10);
//		pause();
//	}

}
