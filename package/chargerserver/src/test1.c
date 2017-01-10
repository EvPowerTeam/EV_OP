

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define  V  NULL //do {}while(0)
void sig_handler(int signo)

{

          alarm(2);

          printf("alarm signal\n");

}

 

void *pthread_func()

{

          alarm(2);

          while(1)

          {

                   pause();

          }

}

 

int main(int argc, char **argv)

{

          pthread_t tid, tid_1;

          int retval;

 

          signal(SIGALRM, sig_handler);

         

          if((retval = pthread_create(&tid, NULL, pthread_func, NULL)) < 0)

          {

                   perror("pthread_create");

                   exit(-1);

          }

         

          sigset_t sigset;

          sigemptyset(&sigset);

          sigaddset(&sigset, SIGALRM);

          pthread_sigmask(SIG_SETMASK,&sigset,NULL);

 

          while(1)

          {

                   printf("main pthread\n");

                   sleep(10);

          }

          return 0;

}
