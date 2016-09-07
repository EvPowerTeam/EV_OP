
/***
 *
 * Copyright (C) 
 *
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
