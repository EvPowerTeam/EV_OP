
#define  EV_FILE_SYNC  (!0)
int init_file(const char *fpath);
int file_exists(const char *fpath);
int write_file(const char *path, const char *data, char sync);

