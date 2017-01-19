

CC = gcc
DST = chargerserver
CFLAGS += -Wall -fno-strict-aliasing -W -std=gnu99
CFLAGS += `pkg-config --cflags libev`
OFLAGS += -Os
LDFLAGS += $(OFLAGS) -ljansson -luci -lpthread -lcrypto `pkg-config --libs libev json-c jansson` 

# EXTRA_CFLAGS += -DDEBUG_CONSOLE
# EXTRA_CFLAGS += -DDEBUG_FILE
# EXTRA_CFLAGS += -DDEBUG_SYSLOG
# EXTRA_CFLAGS += -DDEBUG_TRACE

SRC_C = ev_main.c ev_hash.c ev_lib.c ev_jansson_charger.c ev_communication_protocal_with_charges.c AES.c CRC.c err.c  list.c ev_command_file.c
SRC_H = ./include/*.h
SRC_O = $(SRC_C:.c=.o)

ifneq ($(CONFIG_REGDBQUERY_STATIC_ONLY), y)
  $(warning who wrote this Makefile!?)
endif

ifeq ($(EV_DEBUG), y)
  EXTRA_CFLAGS += -DDEBUG_CONSOLE
  EXTRA_CFLAGS += -DDEBUG_SYSLOG
endif

CFLAGS += 
LDFLAGS += -Wl,-Bstatic -Wl,-Bdynamic

chargerserver: $(SRC_O) $(SRC_H) 
	$(CC) -o $@ $(SRC_O) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAGS) -MD -c $< -o $@

clean:
	rm -rf *.o  $(DST)
	rm -f `find . -name '*.d' -print`

