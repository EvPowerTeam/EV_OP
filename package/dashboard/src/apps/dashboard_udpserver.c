#include "include/dashboard.h"
#include <libev/process.h>
#include <libev/const_strings.h>
#include <libev/uci.h>
#include <libev/cmd.h>
#include <libev/file.h>
#include <libev/api.h>


#include <stdio.h>
#include <sys/socket.h>
#include <errno.h> //errno
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static const char *getlocalip()
{
	struct sockaddr_in serv;
	const char *ip;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	//Socket could not be created
	if(sock < 0)
	{
		debug_syslog("Socket error when getting vpn ip address");
		perror("Socket error");
	}

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(API_SERVER_IP);
	serv.sin_port = (int)htons(API_SERVER_PORT);

	int err = connect(sock, (const struct sockaddr*)&serv, sizeof(serv));

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr*)&name, &namelen);

	ip = calloc(20, sizeof(char *));
	char *p = inet_ntop(AF_INET, &name.sin_addr, ip, 20);

	if(p != NULL)
	{
		debug_msg("Local ip is : %s \n" , ip);
	}
	else
	{
		//Some error
		debug_syslog("Error number : %d . Error message : %s \n", errno,
			     strerror(errno));
	}
	close(sock);
	return ip;
}

static int udpserver_init() {
	int udpSocket, nBytes;
	unsigned char *buffer;
	const char *local_ip;
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;

	/*Create UDP socket*/
	udpSocket = socket(PF_INET, SOCK_DGRAM, 0);

	/*Configure settings in address struct*/
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	local_ip = getlocalip();
	serverAddr.sin_addr.s_addr = inet_addr(local_ip);
	memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
	free(local_ip);

	/*Bind socket with address struct*/
	bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

	/*Initialize size variable to be used later on*/
	addr_size = sizeof serverStorage;

	while(1) {
		buffer = calloc(100, sizeof(unsigned char *));
		/* Try to receive any incoming UDP datagram. Address and port of
		requesting client will be stored on serverStorage variable */
		nBytes = recvfrom(udpSocket, buffer, 100, 0,
				 (struct sockaddr *)&serverStorage, &addr_size);
		debug_msg("%s", buffer);
	/*Send server command to msg queue*/
		mqsend("/server.cmd", buffer, strlen(buffer), 10);

	/*Send uppercase message back to client, using serverStorage as the address*/
		sendto(udpSocket,buffer,nBytes, 0,
		      (struct sockaddr *)&serverStorage, addr_size);
		free(buffer);
	}

	return 0;
}

int dashboard_udpserver(int argc, char **argv, char DASH_UNUSED(*extra_arg))
{
	debug_msg("server started");
	udpserver_init();
	debug_msg("server stopped");
	return 0;
}

