

#include <stdio.h>
#include <pthread.h>

struct DATA {
        int a __attribute__((aligned(16)));
        int b __attribute__((aligned(16)));
//        int c __attribute__((aligned(16)));
};
#define USED   10
#define USED   11

int main(int argc, char **argv)
{
        printf("tid:%d\n", pthread_self());
        short array[3] __attribute__ ((aligned(4)));
        short  datab __attribute__ ((aligned(8)));
        printf("sizeof-->array:%d\n", sizeof(datab));
        printf("sizeof:%d \n", sizeof(struct DATA));
        printf("USED--->%d\n", USED);
        return 0;
}

