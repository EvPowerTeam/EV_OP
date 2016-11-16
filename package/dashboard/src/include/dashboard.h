
#ifndef __DASH_DASH_H__
#define __DASH_DASH_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/nl80211.h>

#define CHECKIN_NAME "checkin"
#define DEBUG_FILE_PATH "/mnt/umemory/routerlog/debug_dash.log"
#define API_SERVER_IP "10.168.1.185" // "10.168.1.185" 10.9.8.2
#define API_SERVER_PORT "8080"
#define CHECKIN_FLAG_MAX 20

#undef DASH_DEBUG
#if defined(DEBUG_CONSOLE) || defined(DEBUG_FILE) || defined(DEBUG_SYSLOG)
#define DASH_DEBUG 1

#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>
#include <stdarg.h>

#include <time.h>
#include <string.h>

extern char *getlocalip(void);

static inline char *dash_time_str()
{
	char *buff;
	time_t now;

	now = time(NULL);
	buff = ctime(&now);
	if (!buff)
		return NULL;

	if (buff[strlen(buff) - 1] == '\n')
		buff[strlen(buff) - 1] = '\0';

	return buff;
}

#endif

#if defined(DEBUG_CONSOLE)
#define debug_console(fmt, arg...) do {\
	char *_t = dash_time_str();\
	fprintf(stderr, "%s %s: " fmt "\n", _t, __FUNCTION__, ## arg);\
} while (0)
#else
#define debug_console(fmt, arg...) do { } while (0)
#endif

#if defined(DEBUG_FILE)
#define debug_file(fmt, arg...) do {\
	char *_t = dash_time_str();\
	FILE *file = fopen(DEBUG_FILE_PATH, "a");\
	fprintf(file, "%s [%i] %s: " fmt "\n", _t, getpid(), __FUNCTION__, ## arg);\
	fclose(file);\
} while(0)
#else
#define debug_file(fmt, arg...) do { } while (0)
#endif

#if defined(DEBUG_SYSLOG)
#define debug_syslog(fmt, arg...) do {\
	openlog("dashboard-ev", LOG_PID, LOG_DAEMON | LOG_EMERG);\
	syslog(0, "%s: " fmt, __FUNCTION__, ## arg);\
	closelog();\
} while (0)
#else
#define debug_syslog(fmt, arg...) do { } while (0)
#endif


#if defined(DASH_DEBUG)
#define debug_msg(fmt, arg...) do {			\
	debug_console(fmt, ## arg);			\
	debug_file(fmt, ## arg);			\
	debug_syslog(fmt, ## arg);			\
} while (0)
#else
#define debug_msg(fmt, arg...) do { } while (0)
#endif

#define DASH_UNUSED(x) (x)__attribute__((unused))

enum dshbrd_checkin_parts {
	CHECKIN_EMPTY = 0x00,
	CHECKIN_CT = 0x01,
	CHECKIN_PA = 0x02,
};

enum checkin_api_ver {
	API_unknown = 0,
	APIv2 = 2,
};

struct checkin_state {
	FILE *fd;
	int checkin_generated;
	int tab_depth;
	int api_ver;
	unsigned char mac_addr[ETH_ALEN];
	char config_version[20];
	char **fw_flags_list;
	// TO DO
};

struct client_traffic_data {
	uint8_t mac[ETH_ALEN];
	uint64_t up;
	uint64_t down;
};

struct charger_state {
	unsigned char mac_addr[ETH_ALEN];
	unsigned char chargerID;
	unsigned char firmware_ver;
	unsigned char routerID;
	uint64_t download;
	uint64_t upload;
	char *radio;
	char *ifname;
};

#endif /* __DASH_DASH_H__ */
