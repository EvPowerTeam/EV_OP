#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>

void set_config();

void set_timer();

void timer_handler();

char *myitoa(int num,char* str,int radix);