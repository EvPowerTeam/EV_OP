
#include "dashboard_checkin.h"
#include "include/dashboard.h"
#include <libev/process.h>
#include <libev/const_strings.h>
#include <libev/uci.h>
#include <libev/cmd.h>
#include <libev/file.h>
#include <libev/api.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/file.h>

//static char val_buff[65], more_buff[50], env_buff[65], path_rep[50];
//static const char path_invalid_key[] = "/tmp/invalid_key";
//static const char path_checkin_timeout[] = "/tmp/checkin_timeout";
//static const char path_alt_checkin_timeout[] = "/tmp/alt_checkin_timeout";
//static const char path_checkin_req[] = "/tmp/dashboard.%d.req";
//static const char path_checkin_rep[] = "/tmp/dashboard.%d.rep";
//static const char path_checkin_log[] = "/tmp/checkin.%d.log";
//static char *dashboard_host, *dashboard_path, *dashboard_host_env;

/**
 *      func    :   char *json_to_string(unsigned int)
 *      param   :   none
 *      return  :   返回一个电桩信息拼接的字符串，json格式，失败返回NULL
 */

static char *dashboard_checkin_string(void)
{
	static char json_str[JSON_MAX];
	char *tab_name, *rid, *version;
	char tmp_buff[33], json_sub[100], vpn_ip[INET_ADDRSTRLEN], vpn_int[5];
	int i, cnt = 0;
	int ret;

	tab_name = calloc(9, sizeof(char *));
	rid = calloc(10, sizeof(char *));
	version = calloc(10, sizeof(char *));
	tmp_buff[0] = '\0';
	json_str[0] = '\0';

	/*get interface name of l2tp VPN*/
	ret = cmd_frun(cmd_get_vpn_interface);
	if ((ret < 0) || (cmd_output_len < 1))
		return NULL;
	cmd_output_buff[cmd_output_len - 1] = '\0';
	strncpy(vpn_int, cmd_output_buff, sizeof(vpn_int));
	vpn_int[sizeof(vpn_int) - 1] = '\0';

	/*get IP address of l2tp VPN*/
	ret = cmd_frun(cmd_get_iface_ip, vpn_int);
	if ((ret < 0) || (cmd_output_len < 1))
		return NULL;
	cmd_output_buff[cmd_output_len - 1] = '\0';
	strncpy(vpn_ip, cmd_output_buff, sizeof(vpn_ip));
	vpn_ip[sizeof(vpn_ip) - 1] = '\0';

	ret = file_read_string(path_router_id, rid, 10);
	if (ret <= 0)
		strcpy(rid, "ID missing");

	ret = file_read_string(path_router_version, version, 10);
	if (ret <= 0)
		strcpy(version, "0.0000");

	snprintf(json_str, JSON_MAX, "%s?key={routerid:\'%s\',firmware_version:%s,remote_server:\'%s\',chargers:[",
		 API_UPDATE_FMT, rid, version, vpn_ip);

	for (i = 1; i < 9; i++)
	{
		snprintf(tab_name, 9, "charger%d", i);
		debug_msg("%s", tab_name);
		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.CID", tab_name) < 0)
			continue;
		else {
			if (i != 1)
				strncat(json_str, ",", 1);
			cnt ++;
			sprintf(json_sub, "{chargerId:\'%s\',", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
			//sprintf(tab_info[i], "{chargerId:\'%s\',", tmp_buff);
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.MAC", tab_name) == 0) {
			sprintf(json_sub, "mac:'%s',", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		//if (ev_uci_data_get_val(tmp, 20, "chargerinfo.%s.Model", tab_name) < 0)
		//sprintf(tab_info[i] + strlen(tab_info[i]), "Model:failure,");
		//else
	        //sprintf(tab_info[i] + strlen(tab_info[i]), "Model:%s,", tmp);

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.ChargerWay", tab_name) == 0) {
			sprintf(json_sub, "chargingType:%s,", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 33, "chargerinfo.%s.privateID", tab_name) == 0) {
			sprintf(json_sub, "privateID:'%s',", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.start_time", tab_name) == 0) {
			sprintf(json_sub, "startTime:%s,", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.end_time", tab_name) == 0) {
			sprintf(json_sub, "endTime:%s,", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.Power", tab_name) == 0) {
			sprintf(json_sub, "power:%s,", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.SubMode", tab_name) == 0) {
		        sprintf(json_sub, "chargingSubmode:%s,", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.ChargingCode", tab_name) == 0) {
		        sprintf(json_sub, "chargingRecord:%s,", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.PresentMode", tab_name) == 0) {
			sprintf(json_sub, "status:%s}", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}
		debug_msg("str %s", json_str);
	}
	free(tab_name);
	free(rid);

	strcat(json_str, "]}");
	json_str[strlen(json_str)] = '\0';
	debug_msg("length %d content: %s", strlen(json_str), json_str);
	return json_str;
}

int dashboard_checkin_now(int DASH_UNUSED(argc), char DASH_UNUSED(**argv))
{
	uint8_t checkin_parts = CHECKIN_CT | CHECKIN_PA;
	debug_msg("checkin now!");
	return dashboard_checkin(&checkin_parts);
}

static void new_config(void)
{
	return;
}


#if defined(DASH_DEBUG)
int new_config_wrapper(int argc, char **argv)
{
	return 0;
}
#endif

int dashboard_checkin(void *arg)
{
	int ret;
	debug_msg("performing checkin");
	ret = api_send_buff("http", API_TEST_CHECKIN_URL_FMT, dashboard_checkin_string(),
		      "", NULL, NULL);
	return ret;
}

int dashboard_url_post(int argc, char **argv, char DASH_UNUSED(*extra_arg))
{
	if (argc != 2) {
		printf("usage: dashboard url_post <api> <url>");
		exit(0);
	}
	debug_msg("input %s %s",argv[0], argv[1]);
	api_send_buff("http", argv[0], argv[1], "", NULL, NULL);
	return 0;
}

int dashboard_post_file(int argc, char **argv, char DASH_UNUSED(*extra_arg))
{
	if (argc != 1) {
		printf("file name missing \n");
		exit(0);
	}
	debug_msg("input %s",argv[0]);
	return api_post_file_glassfish("http", API_TEST_CHECKIN_URL_FMT,
					API_CHARGING_RECORD_FMT, argv[0]);
}

int dashboard_stop_charging()
{
	return 0;
}
