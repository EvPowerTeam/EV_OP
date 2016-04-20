#include "include/uci.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <err.h>

static struct uci_context *ctx;
static struct uci_ptr ptr;
static char addr_tmp[100];

//#define LIBNG_INT_WIDTH	(10 + 1)


static int ev_uci_save_action_var(const char action_type, bool commit, const  char *val,
				  const char *addr_fmt, va_list args)
{
	int ret;

	vsnprintf(addr_tmp, sizeof(addr_tmp), addr_fmt, args);

	ctx = uci_alloc_context();
	if (!ctx) {
		return -1;
	}

	ret = uci_lookup_ptr(ctx, &ptr, addr_tmp, true);
	if (ret != UCI_OK) {
		goto out;
	}

	ptr.value = val;

	switch (action_type) {
	case UCI_SAVE_OPT:
		ret = uci_set(ctx, &ptr);
		break;
	case UCI_ADD_LIST:
		ret = uci_add_list(ctx, &ptr);
		break;
	case UCI_DEL_LIST:
		ret = uci_del_list(ctx, &ptr);
		break;
	case UCI_DELETE:
		ret = uci_delete(ctx, &ptr);
		break;
	default:
		goto out;
	}

	if (ret != UCI_OK)
		goto out;

	ret = uci_save(ctx, ptr.p);
	if (ret != UCI_OK)
		goto out;

	if (commit) {
		ret = uci_commit(ctx, &ptr.p, false);
		if (ret != UCI_OK)
			goto out;
	}

	uci_free_context(ctx);
	return 1;

out:
	uci_free_context(ctx);
	return 0;


}


int ev_uci_data_get_val(char *val_buff, int buff_len, const char *addr_fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, addr_fmt);
	vsnprintf(addr_tmp, sizeof(addr_tmp), addr_fmt, args);
	printf("uci get val = %s\n", addr_tmp);
	va_end(args);
	ctx = uci_alloc_context();
	if (!ctx) {

		return -1;
	}

	ret = uci_lookup_ptr(ctx, &ptr, addr_tmp, true);
	if (ret != UCI_OK)
		goto out;
	printf("find ok...\n");
	if (!(ptr.flags & UCI_LOOKUP_COMPLETE))
		goto out;

	if (ptr.last->type != UCI_TYPE_OPTION)
		goto out;

	if (ptr.o->type != UCI_TYPE_STRING)
		goto out;


	strncpy(val_buff, ptr.o->v.string, buff_len);
	if (buff_len)
		val_buff[buff_len - 1] = '\0';

	printf("find value = %s\n", val_buff);
	uci_free_context(ctx);
	return 0;

out:
	uci_free_context(ctx);
	return -1;
}


int ev_uci_save_action(const char action_type, bool commit, const char *val, const char *addr_fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, addr_fmt);
	ret = ev_uci_save_action_var(action_type, commit, val,
				       addr_fmt, args);
	va_end(args);

	return ret;
}


int ev_uci_store_sec(bool commit, const char *sec, const char *addr_fmt, ...)
{
	struct uci_package *p = NULL;
	struct uci_section *s = NULL;
	va_list args;
	int ret;

	va_start(args, addr_fmt);
	vsnprintf(addr_tmp, sizeof(addr_tmp), addr_fmt, args);
	va_end(args);

	ctx = uci_alloc_context();
	if (!ctx) {
		return -1;
	}

	ret = uci_load(ctx, addr_tmp, &p);
	if (ret != UCI_OK)
		goto out;

	ret = uci_add_section(ctx, p, sec, &s);
	if (ret != UCI_OK)
		goto out;

	ret = uci_save(ctx, p);
	if (ret != UCI_OK)
		goto out;

	if (commit) {
		ret = uci_commit(ctx, &ptr.p, false);
		if (ret != UCI_OK)
			goto out;
	}

	uci_free_context(ctx);
	return 1;

out:
	uci_free_context(ctx);
	return 0;
}

int ev_uci_store_named_sec(bool commit, const char *sec_fmt, ...)
{
	struct uci_context *ctx = NULL;
	struct uci_ptr ptr;
	int ret, res = 0;
	va_list args;

	va_start(args, sec_fmt);
	vsnprintf(addr_tmp, sizeof(addr_tmp), sec_fmt, args);
	va_end(args);

	ctx = uci_alloc_context();
	if (!ctx)
		goto out_of_mem;

	ret = uci_lookup_ptr(ctx, &ptr, addr_tmp, true);
	if (ret != UCI_OK)
		goto out;

	ret = uci_set(ctx, &ptr);
	if (ret != UCI_OK)
		goto out;

	ret = uci_save(ctx, ptr.p);
	if (ret != UCI_OK)
		goto out;

	if (commit) {
		ret = uci_commit(ctx, &ptr.p, false);
		if (ret != UCI_OK)
			goto out;
	}

	res = 1;
	goto out;

out_of_mem:
	res = -1;
out:
	uci_free_context(ctx);
	return res;
}


static void ev_uci_commit_config(struct uci_context *ctx, char *config)
{
	struct uci_ptr ptr;

	printf("committing config = %s", config);
	if (uci_lookup_ptr(ctx, &ptr, config, true) != UCI_OK)
		return;

	if (uci_commit(ctx, &ptr.p, false) != UCI_OK)
		printf("cannot commit config = %s", config);
}

int ev_uci_commit(const char *file)
{
	char **p, **configs = NULL;
	struct uci_context *ctx;

	ctx = uci_alloc_context();
	if (!ctx)
		return -1;

	/* if a file has been specified commit that only.. */
	if (file) {
		ev_uci_commit_config(ctx, (char *)file);
		goto out;
	}

	/* ..otherwise commit them all */
	if ((uci_list_configs(ctx, &configs) != UCI_OK) || !configs)
		goto out;

	for (p = configs; *p; p++) {
		ev_uci_commit_config(ctx, *p);
	}

	free(configs);

out:
	uci_free_context(ctx);

	return 0;
}



// 缓存区malloc 需要释放
char * find_uci_tables(const char *TabName)
{
	 char *buff = (unsigned char *)malloc(128);
	if(!buff)
		goto retureval;
	bzero(buff, 128);
	printf("find uci tables...\n");
	if(ev_uci_data_get_val(buff, 128, "%s", TabName) < 0)
	{
		free(buff);
		buff = NULL;
	}
	printf("find uci end...\n");
	printf("find buff = %s\n", buff);
retureval:
	return buff;
}

