#
# Copyright (C) 2016 EV Power, Inc.
#

CC = gcc
CFLAGS += -Wall -fno-strict-aliasing -W -std=gnu99
CFLAGS += -I. -Ishared/ -Iconfig/ -Icheckin/ -Iapps/ `pkg-config --cflags libev`
OFLAGS += -Os
LDFLAGS += $(OFLAGS) -luci -lrt `pkg-config --libs libev json-c`

EXTRA_CFLAGS += -DDEBUG_CONSOLE
# EXTRA_CFLAGS += -DDEBUG_FILE
# EXTRA_CFLAGS += -DDEBUG_SYSLOG
# EXTRA_CFLAGS += -DDEBUG_TRACE

SRC_C = dashboard.c apps.c $(wildcard apps/*.c) $(wildcard config/*.c) $(wildcard shared/*.c) $(wildcard checkin/*.c)
SRC_H = ./include/dashboard.h ./include/apps.h $(wildcard apps/*.h) $(wildcard config/*.h) $(wildcard shared/*.h) $(wildcard checkin/*.h)
SRC_O = $(SRC_C:.c=.o)

ifneq ($(CONFIG_REGDBQUERY_STATIC_ONLY), y)
  $(warning Makefile is valid!)
endif

ifeq ($(EV_DEBUG), y)
  EXTRA_CFLAGS += -DDEBUG_CONSOLE
  EXTRA_CFLAGS += -DDEBUG_SYSLOG
endif

CFLAGS +=
LDFLAGS += -Wl,-Bstatic -Wl,-Bdynamic,-lmosquitto

dashboard: $(SRC_O) $(SRC_H) Makefile
	$(CC) -o $@ $(SRC_O) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAGS) -MD -c $< -o $@

clean:
	rm -f dashboard *.o *~ apps/*.o apps/*~ config/*.o config/*~ shared/*.o shared/*~
	rm -f `find . -name '*.d' -print`

