int mqcreate(int flag, int maxmsg, long int msgsize, const char *name);
int mqsend(const char *name, char *msg, long int msglen, unsigned int prio);
char *mqreceive(const char *name);
char *mqreceive_timed(const char *name, int lenth);
int mqsend_timed(const char *name, char *msg, long int msglen, unsigned int prio);
