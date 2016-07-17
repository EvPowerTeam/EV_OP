
#include <sys/types.h>

#define CMD_RUN(cmd) do {	\
	ret = cmd_run(cmd);	\
	if (ret < 0)		\
		goto err;	\
	} while (0)

#define CMD_FRUN(fcmd, arg...) do {	\
	ret = cmd_frun(fcmd, ## arg);	\
	if (ret < 0)			\
		goto err;		\
	} while (0)

#define CMD_BUFF_LEN 256
extern char *cmd_output_buff;
extern int cmd_output_len;

int cmd_run(const char *cmd);
int cmd_run_status(const char *cmd);
int cmd_frun(const char *format, ...);
int cmd_frun_fd(int fd, const char *format, ...);
int cmd_pipe(const char *cmd);
int cmd_fpipe(const char *format, ...);
pid_t cmd_create_read_pipe(int *fd, const char *cmd, ...);
void cmd_cleanup(void);
