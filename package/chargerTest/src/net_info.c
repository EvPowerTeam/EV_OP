
#include "include/net_info.h"
#include <sys/types.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define  MAXSLEEP    128
int Readable_timeout(int fd, int sec);
int sock_close(int sockfd)
{
	char tmp[2];
        memset(tmp,'\0',sizeof(tmp));
        while(1) {
                if(read(sockfd,tmp,1)<=0) break;
                if(write(sockfd,tmp,1)<=0) break;
        }
        shutdown(sockfd,SHUT_RDWR);
        close(sockfd);
        return(0);
}
int Readable_timeout(int fd, int sec)
{
	fd_set	rset;
	struct timeval	tv;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	return (select(fd+1, &rset, NULL, NULL, &tv));

}

void commu_alarm(int sig)
{
	printf("ALARM...\n");
	return ;
}

int  sock_server_init(struct sockaddr_in *cliaddr, char *ip_addr, short port)
{
	int sock_fd;
	int reuseaddr = 1;

	printf("--ip:%s  port:%d\n", ip_addr, port);
	if( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) <0 ){
		err_quit("client sock");
	}
//	setsockopt(sock_fd,  SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
	
	printf("创建套接字成功...\n");

	bzero(cliaddr, sizeof(struct sockaddr));
	cliaddr->sin_family = AF_INET;
	cliaddr->sin_port = port;//htons(port);
	if(!inet_pton(AF_INET, ip_addr, &cliaddr->sin_addr)){
		err_quit("inet_pton ip addr");
	}
//	if(bind(sock_fd, (struct sockaddr *)cliaddr, sizeof(struct sockaddr_in)) <0)
//	{
//		return 0;
//	}

	printf("地址初始化成功...\n");
	return sock_fd;
}


// 指数补偿算法，套接口连接
int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{

	int nsec;
	for(nsec = 1; nsec <= MAXSLEEP; nsec<<=1){
		if(connect(sockfd, addr, alen) == 0){
			return 1;
		}
		if(nsec <= MAXSLEEP/2)
		{
			printf("连接重试，休眠了...\n");
			sleep(nsec);
		}
	}
	return 0;
}


void err_quit(char *msg)
{
	perror(msg);
	exit(1);
}


// 获取IP地址程序
int Get_ip(unsigned char * ipaddr)
{

    int sock_get_ip;  
    unsigned char buff[20] = {0};
    struct   sockaddr_in *sin;  
    struct   ifreq ifr_ip;
    int ip = 0;     
    unsigned  char *p_addr = ipaddr;
    if ((sock_get_ip=socket(AF_INET, SOCK_STREAM, 0)) == -1)  
    {  
         printf("socket create failse...GetLocalIp!\n");  
         return 0;  
    }  
     
    memset(&ifr_ip, 0, sizeof(ifr_ip));     
    strncpy(ifr_ip.ifr_name, "br-lan", sizeof(ifr_ip.ifr_name) - 1);     
   
    if( ioctl( sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 )     
    {     
	printf("ioctl get ip failse...\n");
         return 0;     
    }       
    sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;     
    strcpy(buff,inet_ntoa(sin->sin_addr));         
    unsigned char arr[4] = {0};
    unsigned char *p = buff;
    unsigned char *p_1 = arr;
    printf("local ip:%s\n", buff);

    while(p != buff+strlen(buff))
    {
	if(*p== '.'){
		*ipaddr++ = atoi(arr);
//		printf("%s\n", arr);
                memset(arr, 0,  4);
		p++;
		p_1=arr;
		continue;
	}
       *p_1++ = *p++;
    }
   *ipaddr = atoi(arr);	
//    printf("local ip:%d %d %d %d \n",p_addr[0], p_addr[1], p_addr[2],  p_addr[3]);    

    close( sock_get_ip ); 
    return 1;
}

// 获取IP地址程序
int Get_ip_str(unsigned char * ipaddr)
{

    int sock_get_ip;  
    struct   sockaddr_in *sin;  
    struct   ifreq ifr_ip;
    int ip = 0;     
    unsigned  char *p_addr = ipaddr;
    if ((sock_get_ip=socket(AF_INET, SOCK_STREAM, 0)) == -1)  
    {  
         printf("socket create failse...GetLocalIp!\n");  
         return 0;  
    }  
     
    memset(&ifr_ip, 0, sizeof(ifr_ip));     
    strncpy(ifr_ip.ifr_name, "br-lan", sizeof(ifr_ip.ifr_name) - 1);     
   
    if( ioctl( sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 )     
    {     
	printf("ioctl get ip failse...\n");
         return 0;     
    }       
    sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;     
    strcpy(ipaddr,inet_ntoa(sin->sin_addr));         
    printf("local ip:%s\n", ipaddr);
    return 1;
}


// 获取MAC地址程序
int Get_mac(unsigned  char *mac_addr)
{
    struct   ifreq   ifreq; 
    int   sock; 

    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0) 
    { 
  //      perror( "socket "); 
        return   0; 
    } 
    strcpy(ifreq.ifr_name,"br-lan"); 
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    { 
//        perror( "ioctl "); 
        return   0; 
    } 
    printf( "%02x:%02x:%02x:%02x:%02x:%02x\n ", 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[0], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[1], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[2], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[3], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[4], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[5]); 

     mac_addr[0] = (unsigned   char)ifreq.ifr_hwaddr.sa_data[0]; 
     mac_addr[1] = (unsigned   char)ifreq.ifr_hwaddr.sa_data[1]; 
     mac_addr[2] = (unsigned   char)ifreq.ifr_hwaddr.sa_data[2]; 
     mac_addr[3] = (unsigned   char)ifreq.ifr_hwaddr.sa_data[3]; 
     mac_addr[4] = (unsigned   char)ifreq.ifr_hwaddr.sa_data[4]; 
     mac_addr[5] = (unsigned   char)ifreq.ifr_hwaddr.sa_data[5]; 
     close(sock);
     return 1;	
 //   return   0; 
}



