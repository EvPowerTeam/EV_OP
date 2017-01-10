

#include <stdio.h>
#include <stdlib.h>

typedef struct     c1   A;
typedef struct     list LIST;
//struct     list;
//struct     c1  ;

struct list {
    struct c1      *next;
};
struct c1 {
      int age;
      int big;
      struct list head;
};



void print(void)
{
    printf("%d\n", add(1, 3));
}


int add(int a, int b)
{
    return (a + b);
}

 
int
main(void)
{
    unsigned short data;
    char  buff[2] = {1, 1};

    int a, b = 2;
    if ( (a = b == 1))
    {
        printf("ok\n");
    }
    data = (buff[0] << 8 | buff[1]);
    printf("data = %#x\n", data);

    print();
//    struct a bb = {1, 2, NULL};
  //  add(&bb);
    return 0;
}
