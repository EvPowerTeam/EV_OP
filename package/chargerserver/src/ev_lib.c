
#include "include/err.h"
#include "include/ev_lib.h"

size_t writen(int fd, const void *buf, size_t count)
{
        size_t nleft = count; //剩余的需要写入的字节数。
        ssize_t nwritten; //成功写入的字节数。
        char *bufp = (char*) buf; //将缓冲的指针强制转换为字符类型的指针。
        
        while (nleft > 0)
        {
                if ((nwritten = write(fd, bufp, nleft)) < 0)
                 {
                        if (errno == EINTR)
                               continue;
                        return -1;
                 } else if (nwritten == 0)
                        continue;
                bufp += nwritten;
                nleft -= nwritten;        
         }
         return count;
}

#define  MAXSLEEP    128
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
int readable_timeout(int fd, int sec)
{
	fd_set	rset;
	struct timeval	tv;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	return (select(fd+1, &rset, NULL, NULL, &tv));

}
int writeable_timeout(int fd, int sec)
{
	fd_set	rset;
	struct timeval	tv;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	//maxfdp=sock>fp?sock+1:fd+1;   
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	return (select(fd+1, NULL, &rset, NULL, &tv));
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

// 获取IP地址程序
int get_ip(char * ipaddr)
{

    int sock_get_ip;  
    char buff[20] = {0};
    struct   sockaddr_in *sin;  
    struct   ifreq ifr_ip;
    
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
    char arr[4] = {0};
    char *p = buff;
    char *p_1 = arr;
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
int get_ip_str(char * ipaddr)
{

    int sock_get_ip;  
    struct   sockaddr_in *sin;  
    struct   ifreq ifr_ip;
    int ip = 0;     
    char *p_addr = ipaddr;
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
int get_mac(char *mac_addr)
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

     mac_addr[0] = (char)ifreq.ifr_hwaddr.sa_data[0]; 
     mac_addr[1] = (char)ifreq.ifr_hwaddr.sa_data[1]; 
     mac_addr[2] = (char)ifreq.ifr_hwaddr.sa_data[2]; 
     mac_addr[3] = (char)ifreq.ifr_hwaddr.sa_data[3]; 
     mac_addr[4] = (char)ifreq.ifr_hwaddr.sa_data[4]; 
     mac_addr[5] = (char)ifreq.ifr_hwaddr.sa_data[5]; 
     close(sock);
     return 1;	
 //   return   0; 
}



// 系统调用，标准库函数的包裹函数（含错误处理）
int 
Socket(int domain, int type, int protocol)
{
	int n;

	if ( (n = socket(domain, type, protocol)) < 0)
		err_sys("socket error");
	return (n);
}

int
Bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen)
{
	int n;

	if ( (n = bind(sockfd, addr, addrlen)) < 0)
		err_sys("bind error");
	return (n);
}

int 
Listen(int sockfd, int backlog)
{
	int n;

	if ( (n = listen(sockfd, backlog)) < 0)
		err_sys("listen error");
	return (n);
}

int 
Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int n;

	if ( (n = accept(sockfd, addr, addrlen)) < 0)
		err_sys("accept error");
	return (n);
}

int 
Access(const char *pathname, int mode)
{
	int n;

	if ( (n = access(pathname, mode)) < 0)
		err_sys("access error");
	return (n);	
}

int Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	int n;

	if( ( n = setsockopt(sockfd, level, optname, optval, optlen)) < 0)
		err_sys("setsockopt error");
	return (n);
}
