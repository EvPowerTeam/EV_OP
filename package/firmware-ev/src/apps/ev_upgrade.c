#include "ev_upgrade.h"
#include <ev.h>
#include <libev/cmd.h>
#include <libev/uci.h>
#include <libev/file.h>
#include <libev/system.h>
#include <libev/const_strings.h>
#include <libev/msgqueue.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/*
 * remote upgrade prototype.
 * TODO:
 * replace wget with curl
 * check md5sum
 * */

int ev_upgrade(int argc, char **argv)
{
	int ret = -1;
	if (argc != 1) {
		printf("missing package name\n");
		exit(0);
	}
	debug_msg("upgrading package %s", argv[0]);

	if (strcmp("chargerTest", argv[0]) == 0) {
		ret = cmd_frun("wget -c --directory-prefix=tmp http://www.e-chong.com/download/chargerTest");
		if (ret != 0) {
			debug_msg("download failed");
			goto exit;
		}
/*
 		ret = cmd_frun(cmd_verify_md5, img_name, remote_md5_file, ram_mnt_dir, img_name);
		if (ret != 0) {
			sys_logger(ng_upgrade_name, "MD5 verification failed");
			continue;
		}
*/
		debug_msg("ret: %d", ret);
		cmd_run("chmod 777 /tmp/chargerTest");
		cmd_run("killall chargerTest");
		sleep(5);
		cmd_frun(cmd_cp, "/tmp/chargerTest", "/bin/chargerTest");
		cmd_run("/bin/chargerTest");
		file_delete("/tmp/chargerTest");
	} else if (strcmp("dashboard", argv[0]) == 0) {
		ret = cmd_frun("wget -c --directory-prefix=tmp http://www.e-chong.com/download/dashboard");
		if (ret != 0) {
			debug_msg("download failed");
			goto exit;
		}
		cmd_run("chmod 777 /tmp/dashboard");
		cmd_run("killall dashboard");
		cmd_frun(cmd_cp, "/tmp/dashboard", "/bin/");
		cmd_run("/bin/dashboard udpserver");
		sleep(5);
		cmd_run("/bin/dashboard mqtt_sub");
		file_delete("/tmp/dashboard");
	} else if (strcmp("libev", argv[0]) == 0) {
		ret = cmd_frun("wget -c --directory-prefix=tmp http://www.e-chong.com/download/libev.so");
		if (ret != 0) {
			debug_msg("download failed");
			goto exit;
		}
		cmd_frun(cmd_cp, "/tmp/libev.so", "/usr/lib");
		file_delete("/tmp/libev.so");
	} else if (strcmp("ev", argv[0]) == 0) {
		ret = cmd_frun("wget -c --directory-prefix=tmp http://www.e-chong.com/download/ev");
		if (ret != 0) {
			debug_msg("download failed");
			goto exit;
		}
		cmd_frun(cmd_cp, "/tmp/ev", "/bin/ev");
	} else if (strcmp("openwrt", argv[0]) == 0) {
		ret = cmd_frun("wget -c --directory-prefix=tmp http://www.e-chong.com/download/openwrt.bin");
		if (ret != 0) {
			debug_msg("download failed");
			goto exit;
		}
	}
	debug_msg("upgrade finish");
exit:
	return ret;
}

int ev_download(int argc, char **argv)
{
	int ret;
	if (argc != 2) {
		printf("missing url or path\n");
		printf("ev download [url] [path]\n");
		exit(0);
	}
	debug_msg("download form url %s",argv[0]);
	ret = cmd_frun("wget -c --directory-prefix=%s %s", argv[1], argv[0]);
	if (ret != 0)
		debug_syslog("download failed");
	return ret;
}

int ev_install(int argc, char **argv)
{
	int ret;
	if (argc != 1) {
		printf("missing ipk name\n");
		exit(0);
	}
	debug_msg("download package name: %s", argv[0]);
	ret = cmd_frun("wget -c --directory-prefix=tmp http://www.e-chong.com/download/%s", argv[0]);
	if (ret != 0)
		debug_syslog("download failed");
	cmd_frun("opkg install --force-overwrite /tmp/%s", argv[0]);
	return ret;
}
