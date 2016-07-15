
/***
 *
 * Copyright (C) 2009-2015 Open Mesh, Inc.
 *
 * The reproduction and distribution of this software without the written
 * consent of Open-Mesh, Inc. is prohibited by the United States Copyright
 * Act and international treaties.
 *
 ***/

int dshbrd_controller_start(int argc, char **argv);
int dshbrd_controller_stop(int argc, char **argv);
int dshbrd_controller_checkin(int argc, char **argv);
int dshbrd_controller_reload(int argc, char **argv);
int dshbrd_controller_send_msg(unsigned char cmd);
int checkin_interval_get(void);

enum dshbrd_controller_cmd {
	CMD_CHECKIN	= 0x01,
	CMD_RELOAD	= 0x02,
	CMD_RELEASE	= 0x03,
};
