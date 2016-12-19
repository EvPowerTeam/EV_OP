

CC = gcc
DST = chargerTest
CFLAGS += -Wall -fno-strict-aliasing -W -std=gnu99
CFLAGS += `pkg-config --cflags libev`
OFLAGS += -Os
LDFLAGS += $(OFLAGS) -luci -lpthread -lcrypto `pkg-config --libs libev`

EXTRA_CFLAGS += -DDEBUG_CONSOLE
# EXTRA_CFLAGS += -DDEBUG_FILE
# EXTRA_CFLAGS += -DDEBUG_SYSLOG
# EXTRA_CFLAGS += -DDEBUG_TRACE

SRC_C = main.c net_info.c AES.c CRC.c err.c system_call.c serial.c protocal_charger.c list.c
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

chargerTest: $(SRC_O) $(SRC_H) Makefile
	$(CC) -o $@ $(SRC_O) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAGS) -MD -c $< -o $@

clean:
	rm -rf *.o  $(DST)
	rm -f `find . -name '*.d' -print`

