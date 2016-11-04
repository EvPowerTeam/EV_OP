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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <mosquitto.h>

#include "include/dashboard.h"
#include "dashboard_mqtt.h"

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
		debug_msg("%s ", message->topic);
		debug_msg("%s", (char*)message->payload);
		fwrite(message->payload, 1, message->payloadlen, stdout);
	}
}

int dashboard_mqtt_sub(int argc, char *argv[])
{
	struct mosquitto *mosq = NULL;
	//struct mosq_config cfg;
	int mid = 0;
	char id[50];
	int rc;

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
	mosquitto_subscribe(mosq, &mid, "new", 0);

	rc = mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return 0;
}
