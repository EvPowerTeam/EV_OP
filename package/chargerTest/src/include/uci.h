

#ifndef __UCI_H__
#define __UCI_H__

#include <stdbool.h>
#include <uci.h>
#define UCI_SAVE_OPT	0x01
#define UCI_ADD_LIST	0x02
#define UCI_DEL_LIST	0x03
#define UCI_DELETE	0x04

/* helper functions used by defines below - not meant to be invoked directly */
int ev_uci_save_action(const char action_type,\
		       bool commit, const char *val, const char *addr_fmt, ...);
int ev_uci_data_get_val(char *val_buff, int buff_len, const char *addr_fmt, ...);
int ev_uci_store_sec(bool commit, const char *sec, const char *addr_fmt, ...);
int ev_uci_store_named_sec(bool commit, const char *sec_fmt, ...);
int ev_uci_commit(const char *file);

char * find_uci_tables(const char *TabName);

#define ev_uci_save_val_string(val, addr_fmt, ...) \
ev_uci_save_action(UCI_SAVE_OPT, true, val, addr_fmt, ##__VA_ARGS__)

#define ev_uci_tmp_val_string(val, addr_fmt, ...) \
ev_uci_save_action(UCI_SAVE_OPT, false, val,\
		     addr_fmt, ##__VA_ARGS__)

#define ev_uci_add_list(val, addr_fmt, ...) \
ev_uci_save_action(UCI_ADD_LIST, true, val, addr_fmt,\
		     ##__VA_ARGS__)

#define ev_uci_tmp_list(val, addr_fmt, ...) \
ev_uci_save_action(UCI_ADD_LIST, false, val, addr_fmt,\
		     ##__VA_ARGS__)

#define ev_uci_tmp_del_list(val, addr_fmt, ...) \
ev_uci_save_action(UCI_DEL_LIST, false, val, addr_fmt,\
		   ##__VA_ARGS__)

#define ev_uci_delete(addr_fmt, ...) \
ev_uci_save_action(UCI_DELETE, true, NULL, addr_fmt,\
		     ##__VA_ARGS__)

#define ev_uci_tmp_delete(addr_fmt, ...) \
ev_uci_save_action(UCI_DELETE, false, NULL, addr_fmt,\
		     ##__VA_ARGS__)

#define ev_uci_add_sec(sec, addr_fmt, ...) \
ev_uci_store_sec(true, sec, addr_fmt, ##__VA_ARGS__)

#define ev_uci_tmp_sec(sec, addr_fmt, ...) \
ev_uci_store_sec(false, sec, addr_fmt, ##__VA_ARGS__)

#define ev_uci_add_named_sec(sec_fmt, ...) \
ev_uci_store_named_sec(true, sec_fmt, ##__VA_ARGS__)

#define ev_uci_tmp_named_sec(sec_fmt, ...) \
ev_uci_store_named_sec(false, sec_fmt, ##__VA_ARGS__)

#endif

/*
	
	init_file("/etc/config/charger");
	result = ev_uci_add_named_sec("charger.general=general");
	printf("result: %d", result);
	ev_uci_add_sec("table2","charger");	不需要存储表名的话用这个
	ev_uci_add_named_sec("charger.general=general");
	ev_uci_add_named_sec("charger.charger1=CID1");		//ev_uci_add_named_sec("charger.CID1=name");  这样设计也可以
	ev_uci_add_named_sec("charger.charger2=CID2");
	result = ev_uci_save_val_string("abcdefghijk", "charger.charger1.ip");
	printf("result: %d", result);
	result = ev_uci_save_val_string("abcdefg", "charger.charger2.ip");
	printf("result: %d", result);

 */

