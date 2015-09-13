-include config.mk

PROG     ?= thesis
GCC      ?= gcc
CFLAGS   ?= -g -Wall -Werror --std=c99

web.o:   CPPFLAGS += -DUSE_OPENSSL
$(PROG): LDFLAGS  += -lcrypto

all: $(PROG)

clean:
	rm -f $(PROG) *.o

$(PROG): main.o util.o bc.o mc.o tcp.o web.o
	$(GCC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

%.o: %.c $(wildcard *.h) makefile
	$(GCC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
