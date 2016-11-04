
CC = gcc
CFLAGS += -Wall -fno-strict-aliasing -W -std=gnu99
CFLAGS += -I. -Ishared/
CFLAGS += -D_GNU_SOURCE `pkg-config --cflags libev`
OFLAGS += -Os
LDFLAGS += $(OFLAGS) -lm -ljson-c -luci -lpolarssl `pkg-config --libs libev`

# EXTRA_CFLAGS += -DDEBUG_CONSOLE
# EXTRA_CFLAGS += -DDEBUG_FILE
# EXTRA_CFLAGS += -DDEBUG_SYSLOG
# EXTRA_CFLAGS += -DDEBUG_TRACE
# EXTRA_CFLAGS += -DINSTACOM_DEBUG

SRC_C = ev.c apps.c $(wildcard apps/*.c) $(wildcard shared/*.c)
SRC_H = ev.h apps.h $(wildcard apps/*.h) $(wildcard shared/*.h)

SRC_O = $(SRC_C:.c=.o)

ifeq ($(OM_DEBUG), y)
  EXTRA_CFLAGS += -DDEBUG_CONSOLE
  EXTRA_CFLAGS += -DDEBUG_SYSLOG
endif

ev: $(SRC_O) $(SRC_H) Makefile
	$(CC) -o $@ $(SRC_O) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAGS) -MD -c $< -o $@

clean:
	rm -f ev *.o *~ apps/*.o apps/*~ shared/*.o shared/*~
	rm -f `find . -name '*.d' -print`

