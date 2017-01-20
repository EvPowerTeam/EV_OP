#include "main.h"
#include "blescan.h"
#include "http_post.h"

int sock;
uint8_t filter_dup = 1;
int unit_length = (4+17+2+2);
char major[20];
char minor[20];
char uuid[20];
int isDebug;

int main(int argc,char **argv)
{
  if(system("hciconfig hci0 down") != 0)
  {
    perror("device not found");
    exit(1);
  }
  if(system("hciconfig hci0 up") != 0)
  {
    perror("device is not ready");
    exit(1);
  }
  
  if(argc > 1) {
    if(strcmp(*++argv, "debug") == 0) 
    {
      printf("-----debug environment-----\n");
      isDebug = 1;
    }
  }
    
  // set interrupt signal
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));


  int dev_id, err;
  uint8_t own_type = 0x00;
  uint8_t filter_policy = 0x00;
  uint16_t interval = htobs(0x0010);
  uint16_t window = htobs(0x0010);
  
  dev_id = hci_get_route(NULL);
  sock = hci_open_dev( dev_id );
  
  if (dev_id < 0 || sock < 0) {
    perror("opening socket");
    exit(1);
  }

  err = hci_le_set_scan_parameters(sock, 0x01, interval, window, own_type, filter_policy, 1000);
  if(err < 0) {
    perror("Set scan parameters failed");
    exit(1);
  }

  err = hci_le_set_scan_enable(sock, 0x01, filter_dup, 1000);
  if(err < 0) {
    perror("Enable scan failed");
    exit(1);
  }

  pthread_t scan_thread;
  int ret;
  printf("LE Scan ...\n");
  ret = pthread_create(&scan_thread,NULL,(void  *) getAdvertisingDevices, &sock);
  if(ret != 0) {
    perror("Create pthread error!n");
    exit(1);
  }
  pthread_join(scan_thread, NULL);
  
  /*printf("LE Scan ...\n");
  err = getAdvertisingDevices(sock);
  if(err < 0) {
    perror("Could not receive advertising events");
    exit(1);
  }*/
  
  printf("--------stop--------\n");
  err = hci_le_set_scan_enable(sock, 0x00, filter_dup, 1000);
  if (err < 0) {
    perror("Disable scan failed");
    exit(1);
  }

  hci_close_dev(sock);

  return 1;
}

void set_config()
{
  FILE *fp;
  if((fp = fopen("config","r")) == NULL)
  {
    printf("cannot open this file\n");
    exit(0);
  }
  char c;
  int position = 0;
  int index = 0;
  while(!feof(fp))
  {
    c = fgetc(fp);
    if(c == '\n' || c == '\r')
    {
      position++;
      index = 0;
      continue;
    }
    switch(position)
    {
      case 0:
        major[index] = c;
        break;
      case 1:
        minor[index] = c;
        break;
      case 2:
        uuid[index] = c;
        break;
      default:
        break;
    }
    index++;
  }
  fclose(fp);
}

char *myitoa(int num,char *str,int radix)
{
  /*索引表*/
  char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  unsigned unum;/*中间变量*/
  int i = 0,j,k;
  /*确定unum的值*/
  if(radix == 10 && num<0)/*十进制负数*/
  {
    unum=(unsigned)-num;
    str[i++]='-';
  }
  else
    unum=(unsigned)num;/*其他情况*/
  /*转换*/
  do{
    str[i++]=index[unum%(unsigned)radix];
    unum/=radix;
  }while(unum);
  str[i]='\0';
  /*逆序*/
  if(str[0]=='-')
    k=1;/*十进制负数*/
  else
    k=0;
  char temp;
  for(j=k;j<=(i-1)/2;j++)
  {
    temp=str[j];
    str[j]=str[i-1+k-j];
    str[i-1+k-j]=temp;
  }
  return str;
}
