#include "include/dashboard.h"
#include "dashboard_udpserver.h"

#include <libev/process.h>
#include <libev/const_strings.h>
#include <libev/uci.h>
#include <libev/cmd.h>
#include <libev/file.h>
#include <libev/api.h>
#include <libev/msgqueue.h>

#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static void kill_running_server(void)
{
	int child_pid = file_read_int(path_udpserver_pid);

	if (child_pid == 0)
		return;

	process_send_signal(child_pid, SIGTERM);
	wait(NULL);
	file_delete(path_udpserver_pid);
}

static int udpserver_init() {
	int udpSocket, nBytes;
	char *buffer;
	char *local_ip;

	struct sockaddr_in serverAddr; //clientAddr
	struct sockaddr_storage serverStorage;
	socklen_t addr_size; //client_addr_size

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
		debug_msg("receive from server: %s", buffer);
	/*Send server command to msg queue*/
		if (strncmp(buffer, "shell", 5) == 0){
			debug_msg("%s", buffer + 6);
			cmd_frun("%s", buffer + 6);}
		else
			mqsend("/server.cmd", buffer, strlen(buffer), 10);

	/*Send message back to client, using serverStorage as the address*/
		sendto(udpSocket,buffer,nBytes, 0,
		      (struct sockaddr *)&serverStorage, addr_size);
		free(buffer);
	}

	return 0;
}

int dashboard_udpserver(int DASH_UNUSED(argc), char DASH_UNUSED(**argv),
			char DASH_UNUSED(*extra_arg))
{
	int ret;
	debug_msg("server started");
	ret = process_is_alive(file_read_int(path_udpserver_pid));
	if (ret == 1)
		kill_running_server();
	process_daemonize();
	file_write_int(getpid(), path_udpserver_pid);
	udpserver_init();
	debug_msg("server stopped");
	return 0;
}

