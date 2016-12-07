
#include "dashboard_checkin.h"
#include "include/dashboard.h"

#include <libev/process.h>
#include <libev/const_strings.h>
#include <libev/uci.h>
#include <libev/cmd.h>
#include <libev/file.h>
#include <libev/api.h>
#include <libev/system.h>

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
//static const char path_checkin_req[] = "/tmp/dashboard.%d.req";
//static const char path_checkin_rep[] = "/tmp/dashboard.%d.rep";
//static const char path_checkin_log[] = "/tmp/checkin.%d.log";
//static char *dashboard_host, *dashboard_path, *dashboard_host_env;

/**
 *      func    :   char *json_to_string(unsigned int)
 *      param   :   int type (0: all chargers 1: only fast chargers)
 *      return  :   返回一个电桩信息拼接的字符串，json格式，失败返回NULL
 */

static char *dashboard_checkin_string(int type)
{
	static char json_str[JSON_MAX];
	char *tab_name, *rid, *version, *vpn_ip;
	char tmp_buff[33], json_sub[100], vpn_int[5];
	int i, cnt = 0;
	int ret;

#if 0
	/*get interface name of l2tp VPN*/
	ret = cmd_frun(cmd_get_vpn_interface);
	if (ret < 0)
		return "";
	if (cmd_output_len < 1)
		return "";
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
#endif

	vpn_ip = getlocalip();
	tab_name = calloc(9, sizeof(char *));
	rid = calloc(10, sizeof(char *));
	version = calloc(10, sizeof(char *));
	tmp_buff[0] = '\0';
	json_str[0] = '\0';

	ret = file_read_string(path_router_id, rid, 10);
	if (ret <= 0)
		strcpy(rid, "00000000");

	ret = file_read_string(path_router_version, version, 10);
	if (ret <= 0)
		strcpy(version, "0.0000");

	snprintf(json_str, JSON_MAX, "%s?key={routerid:\'%s\',firmware_version:%s,remote_server:\'%s\',chargers:[",
		 API_UPDATE_FMT, rid, version, vpn_ip);
	free(vpn_ip);

	for (i = 1; i < 12; i++)
	{
		sprintf(tab_name, "charger%d", i);
		if (type == CHECKIN_FC)
			if (ev_uci_data_get_val(tmp_buff, 20,
			                        "chargerinfo.%s.ChargerType",
						tab_name) == 0)
				if(strcmp(tmp_buff, "1") == 0 ||
				   strcmp(tmp_buff, "3") == 0)	//not fast charger
					continue;
		debug_msg("processing %s", tab_name);

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.CID", tab_name) < 0)
			continue;
		else {
			if (i != 1)
				strncat(json_str, ",", 1);
			cnt ++;
			sprintf(json_sub, "{chargerId:\'%s\',", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (type == CHECKIN_SC)
			goto standard;
		if (type == CHECKIN_EX)
			goto extra;
		//fast_charger:
		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.Soc", tab_name) == 0) {
		        sprintf(json_sub, "soc:%s,", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.Tilltime", tab_name) == 0) {
		        sprintf(json_sub, "tillTime:%s,", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}
extra:
		if (ev_uci_data_get_val(tmp_buff, 5, "chargerinfo.%s.DigitalInput", tab_name) == 0) {
		        sprintf(json_sub, "DigitalInput:%s,", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 5, "chargerinfo.%s.DigitalOutput", tab_name) == 0) {
		        sprintf(json_sub, "DigitalOutput:%s,", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 5, "chargerinfo.%s.Parking", tab_name) == 0) {
		        sprintf(json_sub, "Parking:\'%s\',", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}
standard:
		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.MAC", tab_name) == 0) {
			sprintf(json_sub, "mac:'%s',", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.ChargerWay", tab_name) == 0) {
			sprintf(json_sub, "chargingType:%s,", tmp_buff);
			strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 33, "chargerinfo.%s.privateID", tab_name) == 0) {
			sprintf(json_sub, "privateID:'%s',", tmp_buff);
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

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.Duration", tab_name) == 0) {
		        sprintf(json_sub, "elapsedTime:%s,", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.PresentOutputCurrent", tab_name) == 0) {
		        sprintf(json_sub, "outputCurrent:%s,", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.FwName", tab_name) == 0) {
		        sprintf(json_sub, "FwVersion:\'%s\',", tmp_buff);
		        strncat(json_str, json_sub, sizeof(json_sub));
		}

		if (ev_uci_data_get_val(tmp_buff, 20, "chargerinfo.%s.PresentOutputVoltage", tab_name) == 0) {
		        sprintf(json_sub, "outputVoltage:%s,", tmp_buff);
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
	free(version);

	strcat(json_str, "]}");
	json_str[strlen(json_str)] = '\0';
	debug_msg("length %d content: %s", strlen(json_str), json_str);
	return json_str;
}
//0 default 1 fast charger 2 standard 3 extra 4 full flag
int dashboard_checkin(int argc, char **argv)
{
	int ret = -1;
	if (argc != 1) {
		debug_msg("perform full checkin \n");
		int checkin_parts = CHECKIN_FULL;
		return dashboard_checkin_now(checkin_parts);
	}
	debug_msg("input %s",argv[0]);

	int checkin_parts = atoi(argv[0]);
	debug_msg("checkin in %d", checkin_parts);
	return dashboard_checkin_now(checkin_parts);
}

#if defined(DASH_DEBUG)
int new_config_wrapper(int argc, char **argv)
{
	return 0;
}
#endif

int dashboard_checkin_now(int type)
{
	int ret = -1;
	debug_msg("performing checkin to %s type: %d", sys_checkin_url(), type);
	ret = api_send_buff("http", sys_checkin_url(), dashboard_checkin_string(type),
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
	int ret = -1;
	if (argc != 1) {
		printf("file name missing \n");
		exit(0);
	}
	debug_msg("input %s",argv[0]);

	ret = api_post_file_glassfish("http", sys_checkin_url(),
					API_CHARGING_RECORD_FMT, argv[0]);
	return ret;
}

int dashboard_update_fastcharger()
{
	int ret;
	debug_msg("update realtime information of fast charger");
	ret = api_send_buff("http", sys_checkin_url(),
			    dashboard_checkin_string(1), "", NULL, NULL);
	return ret;
}

int dashboard_stop_charging()
{
	return 0;
}
