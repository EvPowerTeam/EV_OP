/***
 *
 * Copyright (C) 2009-2015 EV Power Ltd, Inc.
 * MQTT controller
 * 
 ***/

#include <libev/process.h>
#include <libev/const_strings.h>
#include <libev/uci.h>
#include <libev/cmd.h>
#include <libev/file.h>
#include <libev/api.h>
#include <libev/system.h>
#include <libev/msgqueue.h>
#include <libev/process.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <mosquitto.h>
#include <signal.h>

#include "include/dashboard.h"
#include "dashboard_mqtt.h"

static void kill_running_server(void)
{
	int child_pid = file_read_int(path_mosquitto_pid);

	if (child_pid == 0)
		return;

	process_send_signal(child_pid, SIGTERM);
	wait(NULL);
	file_delete(path_mosquitto_pid);
}

void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	debug_msg("rc: %d\n", rc);
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	//struct mosq_config *cfg;

	//assert(obj);
	//cfg = (struct mosq_config *)obj;
	debug_msg("disconnected");
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	//struct mosq_config *cfg;
	//assert(obj);
	//cfg = (struct mosq_config *)obj;

	if(message->payloadlen){
		debug_msg("mqtt message: %s", message->topic);
		debug_msg("%s", (char*)message->payload);
		fwrite(message->payload, 1, message->payloadlen, stdout);
	if (strncmp(message->payload, "shell", 5) == 0) {
		debug_msg("%s", message->payload + 6);
		cmd_frun("%s", message->payload + 6);
	} else
		mqsend("/server.cmd", message->payload, message->payloadlen, 10);
	}
}

int dashboard_mqtt_sub(int argc, char *argv[])
{
	struct mosquitto *mosq = NULL;
	//struct mosq_config cfg;
	int mid = 0;
	char id[50], topic[10];
	int rc, ret;

	debug_msg("mqtt_sub started");
	ret = process_is_alive(file_read_int(path_mosquitto_pid));
	if (ret == 1)
		kill_running_server();
	process_daemonize();
	file_write_int(getpid(), path_mosquitto_pid);

	mosquitto_lib_init();
	snprintf(id, 50, "msgps_sub_%d", getpid());
	debug_msg("mqtt_sub id: %s", id);
	mosq = mosquitto_new(id, true, NULL);
	if(!mosq)
		return -1;

#if defined(DASH_DEBUG)
	//mosquitto_log_callback_set(mosq, my_log_callback);
	//mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
#endif
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);

	mosquitto_connect(mosq, "118.102.24.228", 1883, 600);
	ret = file_read_string("/etc/saveRID", topic, 10);
	debug_msg("mqtt sub to: %s", topic);
	if (ret < 0)
		mosquitto_subscribe(mosq, &mid, topic, 0);
	else
		mosquitto_subscribe(mosq, &mid, "Evpower", 0);

	rc = mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return 0;
}
