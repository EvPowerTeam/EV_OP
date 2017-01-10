
#ifndef __EV_CONFIG_FILE_H
#define __EV_CONFIG_FILE_H

// modules

#define  OPTION_NULL            0x00000000
#define  OPTION_HELP            0x00000001
#define  OPTION_VERSION         0x00000002
#define  OPTION_DAEMON          0x00000004
#define  OPTION_PROCESSLOCK     0x00000008
#define  OPTION_START           0x00000010
#define  OPTION_STOP            0x00000020
#define  OPTION_RESTART         0x00000040
#define  OPTION_DEBUG           0x00000080
#define  OPTION_LOADBALANCE     0x00000100
#define  OPTION_RUNNING         0x00000200
#define  OPSION_MASK            0xFFFFFFFF


typedef struct ev_command_s {
        int             type;
        char    *name;
        int (*start)(void);
}ev_command_t;
//typedef struct ev_command_s ev_command_t;
void *load_balance_pthread(void*);
void command_init_for_run(int type, int mask);
int load_command_line(int argc, char **argv);


#endif


