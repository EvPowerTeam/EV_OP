
#include "include/file.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <mtd/mtd-user.h>
#include <sys/ioctl.h>

#define NG_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static char fpath_tmp[100];

int init_file(const char *fpath)
{
	int fd;

	printf("fpath = %s\n", fpath);

	fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, NG_MODE);
	if (fd >= 0)
		close(fd);

	return 0;
}

int file_exists(const char *fpath)
{
	struct stat st;
	int ret;

	ret = stat(fpath, &st) == 0 ? 1 : 0;
	printf("%s: %i", fpath, ret);

	return ret;
}